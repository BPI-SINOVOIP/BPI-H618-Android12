#!/usr/bin/env python3
#
# Copyright 2019, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import sys
import re
from resource_utils import get_all_resources, get_resources_from_single_file, add_resource_to_set, Resource
from git_utils import has_chassis_changes
from datetime import datetime

if sys.version_info[0] != 3:
    print("Must use python 3")
    sys.exit(1)

# path to 'packages/apps/Car/libs/car-ui-lib/'
ROOT_FOLDER = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../'))
OUTPUT_FILE_PATH = os.path.join(ROOT_FOLDER, 'tests/apitest/')


COPYRIGHT_STR = """Copyright (C) %s The Android Open Source Project

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.""" % (datetime.today().strftime("%Y"))


AUTOGENERATION_NOTICE_STR = """ THIS FILE WAS AUTO GENERATED, DO NOT EDIT MANUALLY. """

"""
Script used to update the 'current.xml' file. This is being used as part of pre-submits to
verify whether resources previously exposed to OEMs are being changed by a CL, potentially
breaking existing customizations.

Example usage: python auto-generate-resources.py current.xml
"""
def main():
    parser = argparse.ArgumentParser(description='Check if any existing resources are modified.')
    parser.add_argument('--sha', help='Git hash of current changes. This script will not run if this is provided and there are no chassis changes.')
    parser.add_argument('-f', '--file', default='current.xml', help='Name of output file.')
    parser.add_argument('-c', '--compare', action='store_true',
                        help='Pass this flag if resources need to be compared.')
    args = parser.parse_args()

    if not has_chassis_changes(args.sha):
        # Don't run because there were no chassis changes
        return

    resources = get_all_resources(os.path.join(ROOT_FOLDER, 'car-ui-lib/src/main/res'))
    check_resource_names(resources, get_resources_from_single_file(os.path.join(OUTPUT_FILE_PATH, 'resource_name_allowed.xml')))

    OVERLAYABLE_OUTPUT_FILE_PATH = os.path.join(ROOT_FOLDER, 'car-ui-lib/src/main/res-overlayable/values/overlayable.xml')
    output_file = args.file or 'current.xml'
    if args.compare:
        old_mapping = get_resources_from_single_file(os.path.join(OUTPUT_FILE_PATH, 'current.xml'))
        compare_resources(old_mapping, resources, os.path.join(OUTPUT_FILE_PATH, 'current.xml'))

        old_mapping = get_resources_from_single_file(OVERLAYABLE_OUTPUT_FILE_PATH)
        add_constraintlayout_resources(resources)
        compare_resources(old_mapping, resources, OVERLAYABLE_OUTPUT_FILE_PATH)
    else:
        generate_current_file(resources, output_file)
        generate_overlayable_file(resources, OVERLAYABLE_OUTPUT_FILE_PATH)

def generate_current_file(resources, output_file='current.xml'):
    resources = sorted(resources, key=lambda x: x.type + x.name)

    # defer importing lxml to here so that people who aren't editing chassis don't have to have
    # lxml installed
    import lxml.etree as etree

    root = etree.Element('resources')

    root.addprevious(etree.Comment(AUTOGENERATION_NOTICE_STR))
    for resource in resources:
        item = etree.SubElement(root, 'public')
        item.set('type', resource.type)
        item.set('name', resource.name)

    data = etree.ElementTree(root)

    with open(os.path.join(OUTPUT_FILE_PATH, output_file), 'wb') as f:
        data.write(f, pretty_print=True, xml_declaration=True, encoding='utf-8')

def generate_overlayable_file(resources, output_file='overlayable.xml'):
    add_constraintlayout_resources(resources)
    resources = sorted(resources, key=lambda x: x.type + x.name)

    # defer importing lxml to here so that people who aren't editing chassis don't have to have
    # lxml installed
    import lxml.etree as etree

    root = etree.Element('resources')

    root.addprevious(etree.Comment(COPYRIGHT_STR))
    root.addprevious(etree.Comment(AUTOGENERATION_NOTICE_STR))

    overlayable = etree.SubElement(root, 'overlayable')
    overlayable.set('name', 'car-ui-lib')

    policy = etree.SubElement(overlayable, 'policy')
    policy.set('type', 'public')

    for resource in resources:
        item = etree.SubElement(policy, 'item')
        item.set('type', resource.type)
        item.set('name', resource.name)

    data = etree.ElementTree(root)

    with open(output_file, 'wb') as f:
        data.write(f, pretty_print=True, xml_declaration=True, encoding='utf-8')

def add_constraintlayout_resources(resources):
    # We need these to be able to use base layouts in RROs
    # This should become unnecessary in S
    # source: https://android.googlesource.com/platform/frameworks/opt/sherpa/+/studio-3.0/constraintlayout/src/main/res/values/attrs.xml
    add_resource_to_set(resources, Resource('layout_optimizationLevel', 'attr'))
    add_resource_to_set(resources, Resource('constraintSet', 'attr'))
    add_resource_to_set(resources, Resource('barrierDirection', 'attr'))
    add_resource_to_set(resources, Resource('constraint_referenced_ids', 'attr'))
    add_resource_to_set(resources, Resource('chainUseRtl', 'attr'))
    add_resource_to_set(resources, Resource('title', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintGuide_begin', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintGuide_end', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintGuide_percent', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintLeft_toLeftOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintLeft_toRightOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintRight_toLeftOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintRight_toRightOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintTop_toTopOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintTop_toBottomOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintBottom_toTopOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintBottom_toBottomOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintBaseline_toBaselineOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintStart_toEndOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintStart_toStartOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintEnd_toStartOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintEnd_toEndOf', 'attr'))
    add_resource_to_set(resources, Resource('layout_goneMarginLeft', 'attr'))
    add_resource_to_set(resources, Resource('layout_goneMarginTop', 'attr'))
    add_resource_to_set(resources, Resource('layout_goneMarginRight', 'attr'))
    add_resource_to_set(resources, Resource('layout_goneMarginBottom', 'attr'))
    add_resource_to_set(resources, Resource('layout_goneMarginStart', 'attr'))
    add_resource_to_set(resources, Resource('layout_goneMarginEnd', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHorizontal_bias', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintVertical_bias', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintWidth_default', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHeight_default', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintWidth_min', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintWidth_max', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintWidth_percent', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHeight_min', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHeight_max', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHeight_percent', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintLeft_creator', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintTop_creator', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintRight_creator', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintBottom_creator', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintBaseline_creator', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintDimensionRatio', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHorizontal_weight', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintVertical_weight', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintHorizontal_chainStyle', 'attr'))
    add_resource_to_set(resources, Resource('layout_constraintVertical_chainStyle', 'attr'))
    add_resource_to_set(resources, Resource('layout_editor_absoluteX', 'attr'))
    add_resource_to_set(resources, Resource('layout_editor_absoluteY', 'attr'))

def check_resource_names(resources, allowed):
    newlist = resources.difference(allowed)
    failed=False
    for resource in newlist:
        resourceType= resource.type
        resourceName = resource.name
        if resourceType == 'attr' and not re.match("^CarUi", resourceName, re.IGNORECASE):
            print(f"Please consider changing {resourceType}/{resourceName} to something like CarUi{resourceName}")
            failed=True
        elif resourceType == 'style' and not re.search("CarUi", resourceName, re.IGNORECASE):
            print(f"Please consider changing {resourceType}/{resourceName} to something like CarUi{resourceName}")
            failed=True
        elif resourceType != 'attr' and resourceType != 'style' and not re.match("^car_ui_", resourceName, re.IGNORECASE):
            print(f"Please consider changing {resourceType}/{resourceName} to something like car_ui_{resourceName}")
            failed=True
    if failed:
        sys.exit(1)

def compare_resources(old_mapping, new_mapping, res_public_file):
    removed = old_mapping.difference(new_mapping)
    added = new_mapping.difference(old_mapping)
    if len(removed) > 0:
        print('Resources removed:\n' + '\n'.join(map(lambda x: str(x), removed)))
    if len(added) > 0:
        print('Resources added:\n' + '\n'.join(map(lambda x: str(x), added)))

    if len(added) + len(removed) > 0:
        print("Some resource have been modified. If this is intentional please " +
              "run 'python3 $ANDROID_BUILD_TOP/packages/apps/Car/libs/car-ui-lib/tests/apitest/" +
              "auto-generate-resources.py' again and submit the new %s" % res_public_file)
        sys.exit(1)

if __name__ == '__main__':
    main()
