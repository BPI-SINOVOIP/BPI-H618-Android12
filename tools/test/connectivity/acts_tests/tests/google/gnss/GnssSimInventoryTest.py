import time
import os
import tempfile

from acts import utils
from acts import signals
from acts.base_test import BaseTestClass
from acts_contrib.test_utils.tel.tel_defines import EventSmsSentSuccess
from acts_contrib.test_utils.tel.tel_test_utils import get_iccid_by_adb
from acts_contrib.test_utils.tel.tel_test_utils import is_sim_ready_by_adb


class GnssSimInventoryTest(BaseTestClass):
    """ GNSS SIM Inventory Tests"""
    def setup_class(self):
        super().setup_class()
        self.ad = self.android_devices[0]
        req_params = ["sim_inventory_recipient", "sim_inventory_ldap"]
        self.unpack_userparams(req_param_names=req_params)

    def check_device_status(self):
        if int(self.ad.adb.shell("settings get global airplane_mode_on")) != 0:
            self.ad.log.info("Force airplane mode off")
            utils.force_airplane_mode(self.ad, False)
        if not is_sim_ready_by_adb(self.ad.log, self.ad):
            raise signals.TestFailure("SIM card is not loaded and ready.")

    def get_imsi(self):
        self.ad.log.info("Get imsi from netpolicy.xml")
        tmp_path = tempfile.mkdtemp()
        self.ad.pull_files("/data/system/netpolicy.xml", tmp_path)
        netpolicy_path = os.path.join(tmp_path, "netpolicy.xml")
        with open(netpolicy_path, "r", encoding="utf-8") as file:
            for line in file.readlines():
                if "subscriberId" in line:
                    imsi = line.split(" ")[2].split("=")[-1].strip('"')
                    return imsi
        raise signals.TestFailure("Fail to get imsi")

    def get_iccid(self):
        iccid = str(get_iccid_by_adb(self.ad))
        if not isinstance(iccid, int):
            self.ad.log.info("Unable to get iccid via adb. Changed to isub.")
            iccid = str(self.ad.adb.shell(
                "dumpsys isub | grep iccid")).split(" ")[4].strip(",")
            return iccid
        raise signals.TestFailure("Fail to get iccid")

    def test_gnss_sim_inventory(self):
        self.check_device_status()
        sms_message = "imsi: %s, iccid: %s, ldap: %s, model: %s, sn: %s" % (
            self.get_imsi(), self.get_iccid(), self.sim_inventory_ldap,
            self.ad.model, self.ad.serial)
        self.ad.log.info(sms_message)
        try:
            self.ad.log.info("Send SMS by SL4A.")
            self.ad.droid.smsSendTextMessage(self.sim_inventory_recipient,
                                             sms_message, True)
            self.ad.ed.pop_event(EventSmsSentSuccess, 10)
        except Exception as e:
            raise signals.TestFailure(e)
