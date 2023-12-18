import os
import tempfile

import common
import sparse_img

OPTIONS = common.OPTIONS

def AssertBootVersion(info):
  info.script.AppendExtra(
      'assert_boot_version(%s);'%(info.info_dict.get("boot_version", "0")))

def GetFex(name, tmpdir):
  path = os.path.join(tmpdir, "IMAGES", name)
  if os.path.exists(path):
    return common.File.FromLocalFile(name, path)
  else:
    print " %s is not exist " %(path)
    return None

def WriteRawFex(info, mount_point, fn):
  info.script.AppendExtra(
      'package_extract_file("%s","%s");'%(fn, mount_point))

VENDOR_PARTITIONS = [
    ('bootloader', 'boot-resource.fex'),
    ('env', 'env.fex'),
    ('vbmeta', 'vbmeta.img'),
    ('vbmeta_system', 'vbmeta_system.img'),
    ('vbmeta_vendor', 'vbmeta_vendor.img'),
    ('dtbo', "dtbo.img"),
]

AB_OTA_PARTITIONS = [
    ('env', 'env.fex'),
    ('bootloader', 'boot-resource.fex'),
]

def UpdateVendorPartitions(info):
  for partition, img in VENDOR_PARTITIONS:
    print('%-25s = %s' % (partition, img))
    img_block = GetFex(img, OPTIONS.target_tmp)
    if img_block:
      info.script.Print('Updating %s into %s partition...' % (img, partition))
      common.ZipWriteStr(info.output_zip, img, img_block.data)
      WriteRawFex(info, '/dev/block/by-name/' + partition, img)

def UpdateABVendorPartitions(info):
  for partition, img in AB_OTA_PARTITIONS:
    print('%-25s = %s' % (partition, img))
    img_block = GetFex(img, OPTIONS.target_tmp)
    if img_block:
      common.ZipWriteStr(info.output_zip, img, img_block.data)

def UpdateBoot(info):
  boot0_nand = GetFex("boot0_nand.fex", OPTIONS.target_tmp)
  if boot0_nand:
    common.ZipWriteStr(info.output_zip, "boot0_nand.fex", boot0_nand.data)
  boot0_sdcard = GetFex("boot0_sdcard.fex", OPTIONS.target_tmp)
  if boot0_sdcard:
    common.ZipWriteStr(info.output_zip, "boot0_sdcard.fex", boot0_sdcard.data)
  uboot = GetFex("u-boot.fex", OPTIONS.target_tmp)
  if uboot:
    common.ZipWriteStr(info.output_zip, "u-boot.fex", uboot.data)
  toc0 = GetFex("toc0.fex", OPTIONS.target_tmp)
  if toc0:
    common.ZipWriteStr(info.output_zip, "toc0.fex", toc0.data)
  toc1 = GetFex("toc1.fex", OPTIONS.target_tmp)
  if toc1:
    common.ZipWriteStr(info.output_zip, "toc1.fex", toc1.data)

def WirteScript(info):
  info.script.Print("Updating boot...")
  info.script.AppendExtra('burnboot();')

# only use in when mbr to gpt UDISK size need to be smaller
def ResizeUdisk(info):
  info.script.AppendExtra('resize2fs("%s");'%("/dev/block/by-name/UDISK"))

def FullOTA_Assertions(info):
  AssertBootVersion(info)

def FullOTA_InstallEnd(info):
  print("pack custom to OTA package...")
  UpdateVendorPartitions(info)
  UpdateBoot(info)
  WirteScript(info)

def IncrementalOTA_Assertions(info):
  AssertBootVersion(info)

def IncrementalOTA_InstallEnd(info):
  print("pack custom to OTA package...")
  UpdateVendorPartitions(info)
  UpdateBoot(info)
  WirteScript(info)

#def AB_OTA_Install(info):
#  UpdateBoot(info)
#  UpdateABVendorPartitions(info)


# The joint list of user image partitions of source and target builds.
# - Items should be added to the list if new dynamic partitions are added.
# - Items should not be removed from the list even if dynamic partitions are
#   deleted. When generating an incremental OTA package, this script needs to
#   know that an image is present in source build but not in target build.
USERIMAGE_PARTITIONS = [
    "product",
]


def GetUserImages(input_tmp, input_zip):
  return {partition: common.GetUserImage(partition, input_tmp, input_zip)
          for partition in USERIMAGE_PARTITIONS
          if os.path.exists(os.path.join(input_tmp,
                                         "IMAGES", partition + ".img"))}


def FullOTA_GetBlockDifferences(info):
  images = GetUserImages(info.input_tmp, info.input_zip)
  return [common.BlockDifference(partition, image)
          for partition, image in images.items()]


def IncrementalOTA_GetBlockDifferences(info):
  source_images = GetUserImages(info.source_tmp, info.source_zip)
  target_images = GetUserImages(info.target_tmp, info.target_zip)

  # Use EmptyImage() as a placeholder for partitions that will be deleted.
  for partition in source_images:
    target_images.setdefault(partition, common.EmptyImage())

  # Use source_images.get() because new partitions are not in source_images.
  return [common.BlockDifference(partition, target_image, source_images.get(partition))
          for partition, target_image in target_images.items()]
