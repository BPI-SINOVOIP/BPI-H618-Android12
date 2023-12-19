#!/usr/bin/python2.7
# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This implements a simple tool to create policy blobs signed with a given
key.  It can create both device and user policies. The output will consist of
two files a policy file and the owner.key file which contains the policy
signature.

The input file is JSON. The root dictionary contains a list under the
key "managed_users". Keys in the root dictionary identify request scopes.
The user-request scope is described by a dictionary that holds two
sub-dictionaries: "mandatory" and "recommended". Both these hold the policy
definitions as key/value stores, their format is identical to what the Linux
implementation reads from /etc.
The device-scope holds the policy-definition directly as key/value stores
in the protobuf-format.

Example:

{
  "google/chromeos/device" : {
      "guest_mode_enabled" : false
  },
  "google/chromeos/user" : {
     "mandatory" : {
      "HomepageLocation" : "http://www.chromium.org",
      "IncognitoEnabled" : false
    },
     "recommended" : {
      "JavascriptEnabled": false
    }
  }
}

"""

import optparse
import os
import re
import sys
import time
import tlslite
import tlslite.api
import tlslite.utils

# The name and availability of the json module varies in python versions.
try:
  import simplejson as json
except ImportError:
  try:
    import json
  except ImportError:
    json = None

import asn1der
import device_management_backend_pb2 as dm
import cloud_policy_pb2 as cp
import chrome_device_policy_pb2 as dp

# ASN.1 object identifier for PKCS#1/RSA.
PKCS1_RSA_OID = '\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01'

def SetProtobufMessageField(group_message, field, field_value):
  '''Sets a field in a protobuf message.

  Args:
    group_message: The protobuf message.
    field: The field of the message to set, it shuold be a member of
        group_message.DESCRIPTOR.fields.
    field_value: The value to set.
  '''
  if field.label == field.LABEL_REPEATED:
    assert type(field_value) == list
    entries = group_message.__getattribute__(field.name)
    for list_item in field_value:
      entries.append(list_item)
    return
  elif field.type == field.TYPE_BOOL:
    assert type(field_value) == bool
  elif field.type == field.TYPE_STRING:
    assert type(field_value) == str or type(field_value) == unicode
  elif field.type == field.TYPE_INT64:
    assert type(field_value) == int
  elif (field.type == field.TYPE_MESSAGE and
        field.message_type.name == 'StringList'):
    assert type(field_value) == list
    entries = group_message.__getattribute__(field.name).entries
    for list_item in field_value:
      entries.append(list_item)
    return
  else:
    raise Exception('Unknown field type %s' % field.type)
  group_message.__setattr__(field.name, field_value)

def GatherDevicePolicySettings(settings, policies):
  '''Copies all the policies from a dictionary into a protobuf of type
  CloudDeviceSettingsProto.

  Args:
    settings: The destination ChromeDeviceSettingsProto protobuf.
    policies: The source dictionary containing policies in JSON format.
  '''
  for group in settings.DESCRIPTOR.fields:
    # Create protobuf message for group.
    group_message = eval('dp.' + group.message_type.name + '()')
    # Indicates if at least one field was set in |group_message|.
    got_fields = False
    # Iterate over fields of the message and feed them from the
    # policy config file.
    for field in group_message.DESCRIPTOR.fields:
      field_value = None
      if field.name in policies:
        got_fields = True
        field_value = policies[field.name]
        SetProtobufMessageField(group_message, field, field_value)
    if got_fields:
      settings.__getattribute__(group.name).CopyFrom(group_message)

def GatherUserPolicySettings(settings, policies):
  '''Copies all the policies from a dictionary into a protobuf of type
  CloudPolicySettings.

  Args:
    settings: The destination: a CloudPolicySettings protobuf.
    policies: The source: a dictionary containing policies under keys
        'recommended' and 'mandatory'.
  '''
  for group in settings.DESCRIPTOR.fields:
    # Create protobuf message for group.
    group_message = eval('cp.' + group.message_type.name + '()')
    # We assume that this policy group will be recommended, and only switch
    # it to mandatory if at least one of its members is mandatory.
    group_message.policy_options.mode = cp.PolicyOptions.RECOMMENDED
    # Indicates if at least one field was set in |group_message|.
    got_fields = False
    # Iterate over fields of the message and feed them from the
    # policy config file.
    for field in group_message.DESCRIPTOR.fields:
      field_value = None
      if field.name in policies['mandatory']:
        group_message.policy_options.mode = cp.PolicyOptions.MANDATORY
        field_value = policies['mandatory'][field.name]
      elif field.name in policies['recommended']:
        field_value = policies['recommended'][field.name]
      if field_value != None:
        got_fields = True
        SetProtobufMessageField(group_message, field, field_value)
    if got_fields:
      settings.__getattribute__(group.name).CopyFrom(group_message)

def ProcessCloudPolicy(policy_type,
                       policy_def, policy_key,
                       username,
                       output_path):
  """Creates a policy blob.

  Encodes the policy into protobuf representation, signs it and saves it.

  Args:
    policy_type: can be 'google/chromeos/user' or 'google/chromeos/device'.
    policy_def: The JSON file containing the policy definition.
    policy_key: A private key to be used to sign the blob.
    username: Username to be integrated in the policy blob.
    output_path: A directory where to put the output files.
  """
  policy = json.loads(open(policy_def).read())
  policy_value = ''
  if (policy_type in policy):
    if policy_type == 'google/chromeos/user':
      settings = cp.CloudPolicySettings()
      GatherUserPolicySettings(settings, policy[policy_type])
      policy_value = settings.SerializeToString()
    elif policy_type == 'google/chromeos/device':
      settings = dp.ChromeDeviceSettingsProto()
      GatherDevicePolicySettings(settings, policy[policy_type])
      policy_value = settings.SerializeToString()

  key = tlslite.api.parsePEMKey(open(policy_key).read(), private=True)

  algorithm = asn1der.Sequence(
      [ asn1der.Data(asn1der.OBJECT_IDENTIFIER, PKCS1_RSA_OID),
        asn1der.Data(asn1der.NULL, '') ])
  rsa_pubkey = asn1der.Sequence([ asn1der.Integer(key.n),
                                  asn1der.Integer(key.e) ])
  pubkey = asn1der.Sequence([ algorithm, asn1der.Bitstring(rsa_pubkey) ])
  key_version = 1

  # Fill the policy data protobuf.
  policy_data = dm.PolicyData()
  policy_data.policy_type = policy_type
  policy_data.timestamp = int(time.time() * 1000)
  policy_data.request_token = "DEV_TOKEN"
  policy_data.policy_value = policy_value
  policy_data.machine_name = "MEAN_MACHINE"
  policy_data.public_key_version = 1
  policy_data.username = username
  policy_data.device_id = "1337_1D"
  signed_data = policy_data.SerializeToString()

  response = dm.DeviceManagementResponse()
  fetch_response = response.policy_response.responses.add()
  fetch_response.policy_data = signed_data
  fetch_response.policy_data_signature = bytes(
      key.hashAndSign(signed_data))
  fetch_response.new_public_key = pubkey

  open("%s/policy" % output_path,"wb").
      write(fetch_response.SerializeToString());
  open("%s/owner.key" % output_path,"wb").write(pubkey);

def main(options):
  ProcessCloudPolicy(options.policy_type,
                     options.policy_def, options.policy_key,
                     options.policy_user,
                     options.output_path);

if __name__ == '__main__':
  option_parser = optparse.OptionParser()
  option_parser.add_option('-k', '--policy-key', default="mykey",
                           dest='policy_key',
                           help='Specify a path to a PEM-encoded private key '
                           'to use for policy signing.')
  option_parser.add_option('-p', '--policy-def', default="device_management",
                           dest='policy_def',
                           help='Specify a path to a PEM-encoded private key '
                           'to use for policy signing.')
  option_parser.add_option('-u', '--policy-user', default='user@example.com',
                           dest='policy_user',
                           help='Specify the user name the server should '
                           'report back to the client as the user owning the '
                           'token used for making the policy request.')
  option_parser.add_option('-o', '--output-path', default='.',
                           dest='output_path',
                           help='Specifies the directory to output policy '
                           'files to.')
  option_parser.add_option('-t', '--type', default='google/chromeos/device',
                           dest='policy_type',
                           help='Specifies the type of policy to create.')
  options, args = option_parser.parse_args()

  sys.exit(main(options))
