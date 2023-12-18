#!/usr/bin/env python3
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
import re
from acts.controllers.cellular_lib import LteSimulation


class LteCaSimulation(LteSimulation.LteSimulation):
    """ Carrier aggregation LTE simulation. """

    # Dictionary of lower DL channel number bound for each band.
    LOWEST_DL_CN_DICTIONARY = {
        1: 0,
        2: 600,
        3: 1200,
        4: 1950,
        5: 2400,
        6: 2650,
        7: 2750,
        8: 3450,
        9: 3800,
        10: 4150,
        11: 4750,
        12: 5010,
        13: 5180,
        14: 5280,
        17: 5730,
        18: 5850,
        19: 6000,
        20: 6150,
        21: 6450,
        22: 6600,
        23: 7500,
        24: 7700,
        25: 8040,
        26: 8690,
        27: 9040,
        28: 9210,
        29: 9660,
        30: 9770,
        31: 9870,
        32: 36000,
        33: 36200,
        34: 36350,
        35: 36950,
        36: 37550,
        37: 37750,
        38: 38250,
        39: 38650,
        40: 39650,
        41: 41590,
        42: 45590,
        66: 66436
    }

    # Simulation config keywords contained in the test name
    PARAM_CA = 'ca'

    # Test config keywords
    KEY_FREQ_BANDS = "freq_bands"

    def __init__(self, simulator, log, dut, test_config, calibration_table):
        """ Initializes the simulator for LTE simulation with carrier
        aggregation.

        Loads a simple LTE simulation enviroment with 5 basestations.

        Args:
            simulator: the cellular instrument controller
            log: a logger handle
            dut: a device handler implementing BaseCellularDut
            test_config: test configuration obtained from the config file
            calibration_table: a dictionary containing path losses for
                different bands.

        """

        super().__init__(simulator, log, dut, test_config, calibration_table)

        # Create a configuration object for each base station and copy initial
        # settings from the PCC base station.
        self.bts_configs = [self.primary_config]

        for bts_index in range(1, self.simulator.LTE_MAX_CARRIERS):
            new_config = self.BtsConfig()
            new_config.incorporate(self.primary_config)
            self.simulator.configure_bts(new_config, bts_index)
            self.bts_configs.append(new_config)

        # Get LTE CA frequency bands setting from the test configuration
        if self.KEY_FREQ_BANDS not in test_config:
            self.log.warning("The key '{}' is not set in the config file. "
                             "Setting to null by default.".format(
                                 self.KEY_FREQ_BANDS))

        self.freq_bands = test_config.get(self.KEY_FREQ_BANDS, True)

    def setup_simulator(self):
        """ Do initial configuration in the simulator. """
        self.simulator.setup_lte_ca_scenario()

    def parse_parameters(self, parameters):
        """ Configs an LTE simulation with CA using a list of parameters.

        Args:
            parameters: list of parameters
        """

        # Get the CA band configuration

        values = self.consume_parameter(parameters, self.PARAM_CA, 1)

        if not values:
            raise ValueError(
                "The test name needs to include parameter '{}' followed by "
                "the CA configuration. For example: ca_3c7c28a".format(
                    self.PARAM_CA))

        # Carrier aggregation configurations are indicated with the band numbers
        # followed by the CA classes in a single string. For example, for 5 CA
        # using 3C 7C and 28A the parameter value should be 3c7c28a.
        ca_configs = re.findall(r'(\d+[abcABC])', values[1])

        if not ca_configs:
            raise ValueError(
                "The CA configuration has to be indicated with one string as "
                "in the following example: ca_3c7c28a".format(self.PARAM_CA))

        # Apply the carrier aggregation combination
        self.simulator.set_ca_combination(ca_configs)

        # Save the bands to the bts config objects
        bts_index = 0
        for ca in ca_configs:
            ca_class = ca[-1]
            band = ca[:-1]

            self.bts_configs[bts_index].band = band
            bts_index += 1

            if ca_class.upper() == 'B' or ca_class.upper() == 'C':
                # Class B and C means two carriers with the same band
                self.bts_configs[bts_index].band = band
                bts_index += 1

        # Count the number of carriers in the CA combination
        self.num_carriers = 0
        for ca in ca_configs:
            ca_class = ca[-1]
            # Class C means that there are two contiguous carriers, while other
            # classes are a single one.
            if ca_class.upper() == 'C':
                self.num_carriers += 2
            else:
                self.num_carriers += 1

        # Create an array of configuration objects to set up the base stations.
        new_configs = [self.BtsConfig() for _ in range(self.num_carriers)]

        # Get the bw for each carrier
        # This is an optional parameter, by default the maximum bandwidth for
        # each band will be selected.

        values = self.consume_parameter(parameters, self.PARAM_BW,
                                        self.num_carriers)

        bts_index = 0

        for ca in ca_configs:

            band = int(ca[:-1])
            ca_class = ca[-1]

            if values:
                bw = int(values[1 + bts_index])
            else:
                bw = max(self.allowed_bandwidth_dictionary[band])

            new_configs[bts_index].bandwidth = bw
            bts_index += 1

            if ca_class.upper() == 'C':

                new_configs[bts_index].bandwidth = bw

                # Calculate the channel number for the second carrier to be
                # contiguous to the first one
                new_configs[bts_index].dl_channel = int(
                    self.LOWEST_DL_CN_DICTIONARY[int(band)] + bw * 10 - 2)

                bts_index += 1

        # Get the TM for each carrier
        # This is an optional parameter, by the default value depends on the
        # MIMO mode for each carrier

        tm_values = self.consume_parameter(parameters, self.PARAM_TM,
                                           self.num_carriers)

        # Get the MIMO mode for each carrier

        mimo_values = self.consume_parameter(parameters, self.PARAM_MIMO,
                                             self.num_carriers)

        if not mimo_values:
            raise ValueError(
                "The test parameter '{}' has to be included in the "
                "test name followed by the MIMO mode for each "
                "carrier separated by underscores.".format(self.PARAM_MIMO))

        if len(mimo_values) != self.num_carriers + 1:
            raise ValueError(
                "The test parameter '{}' has to be followed by "
                "a number of MIMO mode values equal to the number "
                "of carriers being used.".format(self.PARAM_MIMO))

        for bts_index in range(self.num_carriers):

            # Parse and set the requested MIMO mode

            for mimo_mode in LteSimulation.MimoMode:
                if mimo_values[bts_index + 1] == mimo_mode.value:
                    requested_mimo = mimo_mode
                    break
            else:
                raise ValueError(
                    "The mimo mode must be one of %s." %
                    {elem.value
                     for elem in LteSimulation.MimoMode})

            if (requested_mimo == LteSimulation.MimoMode.MIMO_4x4
                    and not self.simulator.LTE_SUPPORTS_4X4_MIMO):
                raise ValueError("The test requires 4x4 MIMO, but that is not "
                                 "supported by the MD8475A callbox.")

            new_configs[bts_index].mimo_mode = requested_mimo

            # Parse and set the requested TM

            if tm_values:
                for tm in LteSimulation.TransmissionMode:
                    if tm_values[bts_index + 1] == tm.value[2:]:
                        requested_tm = tm
                        break
                else:
                    raise ValueError(
                        "The TM must be one of %s." %
                        {elem.value
                         for elem in LteSimulation.MimoMode})
            else:
                # Provide default values if the TM parameter is not set
                if requested_mimo == LteSimulation.MimoMode.MIMO_1x1:
                    requested_tm = LteSimulation.TransmissionMode.TM1
                else:
                    requested_tm = LteSimulation.TransmissionMode.TM3

            new_configs[bts_index].transmission_mode = requested_tm

            self.log.info("Cell {} will be set to {} and {} MIMO.".format(
                bts_index + 1, requested_tm.value, requested_mimo.value))

        # Get uplink power

        ul_power = self.get_uplink_power_from_parameters(parameters)

        # Power is not set on the callbox until after the simulation is
        # started. Saving this value in a variable for later
        self.sim_ul_power = ul_power

        # Get downlink power

        dl_power = self.get_downlink_power_from_parameters(parameters)

        # Power is not set on the callbox until after the simulation is
        # started. Saving this value in a variable for later
        self.sim_dl_power = dl_power

        # Setup scheduling mode

        values = self.consume_parameter(parameters, self.PARAM_SCHEDULING, 1)

        if not values:
            scheduling = LteSimulation.SchedulingMode.STATIC
            self.log.warning(
                "The test name does not include the '{}' parameter. Setting to "
                "{} by default.".format(scheduling.value,
                                        self.PARAM_SCHEDULING))
        else:
            for scheduling_mode in LteSimulation.SchedulingMode:
                if values[1].upper() == scheduling_mode.value:
                    scheduling = scheduling_mode
                    break
            else:
                raise ValueError(
                    "The test name parameter '{}' has to be followed by one of "
                    "{}.".format(
                        self.PARAM_SCHEDULING,
                        {elem.value
                         for elem in LteSimulation.SchedulingMode}))

        for bts_index in range(self.num_carriers):
            new_configs[bts_index].scheduling_mode = scheduling

        if scheduling == LteSimulation.SchedulingMode.STATIC:

            values = self.consume_parameter(parameters, self.PARAM_PATTERN, 2)

            if not values:
                self.log.warning(
                    "The '{}' parameter was not set, using 100% RBs for both "
                    "DL and UL. To set the percentages of total RBs include "
                    "the '{}' parameter followed by two ints separated by an "
                    "underscore indicating downlink and uplink percentages.".
                    format(self.PARAM_PATTERN, self.PARAM_PATTERN))
                dl_pattern = 100
                ul_pattern = 100
            else:
                dl_pattern = int(values[1])
                ul_pattern = int(values[2])

            if (dl_pattern, ul_pattern) not in [(0, 100), (100, 0),
                                                (100, 100)]:
                raise ValueError(
                    "Only full RB allocation for DL or UL is supported in CA "
                    "sims. The allowed combinations are 100/0, 0/100 and "
                    "100/100.")

            for bts_index in range(self.num_carriers):

                # Look for a DL MCS configuration in the test parameters. If it
                # is not present, use a default value.
                dlmcs = self.consume_parameter(parameters, self.PARAM_DL_MCS,
                                               1)
                if dlmcs:
                    mcs_dl = int(dlmcs[1])
                else:
                    self.log.warning(
                        'The test name does not include the {} parameter. '
                        'Setting to the max value by default'.format(
                            self.PARAM_DL_MCS))

                    if self.dl_256_qam and new_configs[
                            bts_index].bandwidth == 1.4:
                        mcs_dl = 26
                    elif (not self.dl_256_qam
                          and self.primary_config.tbs_pattern_on
                          and new_configs[bts_index].bandwidth != 1.4):
                        mcs_dl = 28
                    else:
                        mcs_dl = 27

                # Look for an UL MCS configuration in the test parameters. If it
                # is not present, use a default value.
                ulmcs = self.consume_parameter(parameters, self.PARAM_UL_MCS,
                                               1)
                if ulmcs:
                    mcs_ul = int(ulmcs[1])
                else:
                    self.log.warning(
                        'The test name does not include the {} parameter. '
                        'Setting to the max value by default'.format(
                            self.PARAM_UL_MCS))

                    if self.ul_64_qam:
                        mcs_ul = 28
                    else:
                        mcs_ul = 23

                dl_rbs, ul_rbs = self.allocation_percentages_to_rbs(
                    new_configs[bts_index].bandwidth,
                    new_configs[bts_index].transmission_mode, dl_pattern,
                    ul_pattern)

                new_configs[bts_index].dl_rbs = dl_rbs
                new_configs[bts_index].ul_rbs = ul_rbs
                new_configs[bts_index].dl_mcs = mcs_dl
                new_configs[bts_index].ul_mcs = mcs_ul

        # Setup the base stations with the obtained configurations and then save
        # these parameters in the current configuration objects
        for bts_index in range(len(new_configs)):
            self.simulator.configure_bts(new_configs[bts_index], bts_index)
            self.bts_configs[bts_index].incorporate(new_configs[bts_index])

        # Now that the band is set, calibrate the link for the PCC if necessary
        self.load_pathloss_if_required()

    def maximum_downlink_throughput(self):
        """ Calculates maximum downlink throughput as the sum of all the active
        carriers.
        """
        return sum(
            self.bts_maximum_downlink_throughtput(self.bts_configs[bts_index])
            for bts_index in range(self.num_carriers))

    def start(self):
        """ Set the signal level for the secondary carriers, as the base class
        implementation of this method will only set up downlink power for the
        primary carrier component.

        After that, attaches the secondary carriers."""

        super().start()

        if self.sim_dl_power:
            self.log.info('Setting DL power for secondary carriers.')

            for bts_index in range(1, self.num_carriers):
                new_config = self.BtsConfig()
                new_config.output_power = self.calibrated_downlink_rx_power(
                    self.bts_configs[bts_index], self.sim_dl_power)
                self.simulator.configure_bts(new_config, bts_index)
                self.bts_configs[bts_index].incorporate(new_config)

        self.simulator.lte_attach_secondary_carriers(self.freq_bands)
