/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __FACEAUTH_SHARED_H__
#define __FACEAUTH_SHARED_H__
#define INPUT_IMAGE_WIDTH 480
#define INPUT_IMAGE_HEIGHT 640
#define INPUT_IMAGE_SIZE (INPUT_IMAGE_WIDTH * INPUT_IMAGE_HEIGHT)
#define DISABLE_GAZE_DEPRECATED (1ULL << 0)
#define SECURE_CAMERA_DATA (1ULL << 2)
#define REQUIRE_ATTENTION (1ULL << 3)
#define MAX_CACHE_ENROLLMENT 1536
#define MAX_ENROLLMENT 20
#define MAX_NUM_USERS 4
#define ERROR_NO_ERROR 0
#define ERROR_GENERAL_ERROR - 1
#define ERROR_INTERNAL_CITADEL_INVALID_ARGS - 32
#define ERROR_INTERNAL_CITADEL_TIMEOUT - 33
#define ERROR_INTERNAL_CITADEL_SFS_FULL - 34
#define ERROR_INTERNAL_CITADEL_SFS_INTERNAL - 35
#define ERROR_INTERNAL_CITADEL_ERR_THROTTLE - 36
#define ERROR_INTERNAL_CITADEL_SESSION_ID - 37
#define ERROR_INTERNAL_CITADEL_CRC - 38
#define ERROR_INTERNAL_CITADEL_NONSECURE_MODE - 39
#define ERROR_INTERNAL_CITADEL_INVALID_TOKEN - 40
#define ERROR_INTERNAL_CITADEL_RECALIBRATE - 41
#define ERROR_INTERNAL_CITADEL_VERSION - 42
#define ERROR_INTERNAL_CITADEL_ERR_UNKNOWN - 43
#define ERROR_INTERNAL_CITADEL_INVALID_CHALLENGE - 44
#define ERROR_INTERNAL_CITADEL_INVALID_USER_ID - 45
#define ERROR_INTERNAL_CITADEL_EMBEDDING_VERSION - 46
#define ERROR_INTERNAL_CITADEL_EMBEDDING_DOWNGRADE - 47
#define ERROR_CITADEL_INVALID_ARGS - 48
#define ERROR_CITADEL_TIMEOUT - 49
#define ERROR_CITADEL_CACHE_FULL - 50
#define ERROR_CITADEL_CACHE_INVALID_INDEX - 51
#define ERROR_CITADEL_INVALID_PROFILE - 52
#define ERROR_CITADEL_EARLY_FAIL - 53
#define ERROR_CITADEL_SESSION_ID - 54
#define ERROR_CITADEL_CRC - 55
#define ERROR_CITADEL_UNSUPPORTED - 56
#define ERROR_INTERNAL_CITADEL_CHALLENGE_EXPIRED - 57
#define ERROR_INTERNAL_CITADEL_WIPED_USER - 58
#define ERROR_INTERNAL_CITADEL_INVALID_FACENET - 59
#define ERROR_INTERNAL_CITADEL_INVALID_DEPTHID - 60
#define ERROR_DEPTH_IPU_TIMEOUT - 64
#define ERROR_SKIN_INVALID_LAYER_SIZE - 65
#define ERROR_FSSD_BUFFER_SIZE_MISMATCH - 66
#define ERROR_SKIN_INVALID_PATCH_SIZE - 67
#define ERROR_SKIN_INVALID_FACE_SIZE - 68
#define ERROR_DOWNSCALE_IPU_TIMEOUT - 69
#define ERROR_AFFINE_COMBINED_IPU_TIMEOUT - 70
#define ERROR_FACENET_BUFFER_SIZE_MISMATCH - 71
#define ERROR_DEPTHID_BUFFER_SIZE_MISMATCH - 72
#define ERROR_GAZENET_BUFFER_SIZE_MISMATCH - 73
#define ERROR_SKIN_BUFFER_SIZE_MISMATCH - 74
#define ERROR_MAX_THRESHOLD_FACES - 75
#define ERROR_INSUFFICIENT_ALLOCATOR_MEMORY - 76
#define ERROR_IPU_TIMEOUT - 77
#define ERROR_REJECT_USERS_FULL - 78
#define ERROR_REJECT_PROFILES_FULL - 79
#define ERROR_REJECT_MAX_PROFILES_PER_USER - 80
#define ERROR_UNSUPPORTED_COMMAND - 81
#define ERROR_FW_DRIVER_SYNC_ERROR - 84
#define ERROR_UNEXPECTED_COMMAND - 85
#define CMD_IS_VALID(cmd) ((cmd) < COMMAND_COUNT)
typedef enum _workload_status {
  WORKLOAD_STATUS_NO_STATUS = 0,
  WORKLOAD_STATUS_PASS,
  WORKLOAD_STATUS_REJECT_NO_FACE,
  WORKLOAD_STATUS_REJECT_MAX_FACE_CAPACITY,
  WORKLOAD_STATUS_REJECT_NO_ATTENTION,
  WORKLOAD_STATUS_REJECT_TOO_SMALL,
  WORKLOAD_STATUS_REJECT_CLIPPED_LEFT,
  WORKLOAD_STATUS_REJECT_CLIPPED_RIGHT,
  WORKLOAD_STATUS_REJECT_CLIPPED_TOP,
  WORKLOAD_STATUS_REJECT_CLIPPED_BOTTOM,
  WORKLOAD_STATUS_REJECT_TOO_DARK,
  WORKLOAD_STATUS_REJECT_TOO_BRIGHT,
  WORKLOAD_STATUS_REJECT_TILT_ANGLE,
  WORKLOAD_STATUS_REJECT_PAN_ANGLE,
  WORKLOAD_STATUS_REJECT_TOO_CLOSE,
  WORKLOAD_STATUS_REJECT_TOO_FAR,
  WORKLOAD_STATUS_REJECT_USERS_FULL,
  WORKLOAD_STATUS_REJECT_PROFILES_FULL,
  WORKLOAD_STATUS_REJECT_INVALID_DEPTH,
  WORKLOAD_STATUS_REJECT_SKIN,
  WORKLOAD_STATUS_REJECT_SPOOF,
  WORKLOAD_STATUS_HARD_REJECT,
  WORKLOAD_STATUS_ERROR,
  WORKLOAD_STATUS_REJECT_FRAME_SELECTION,
  WORKLOAD_STATUS_COUNT,
  WORKLOAD_STATUS_INTMAX = 0xffffffff
} WorkloadStatus;
typedef enum _faceauth_input_commands {
  COMMAND_NONE = 0,
  COMMAND_ENROLL,
  COMMAND_ENROLL_COMPLETE,
  COMMAND_ERASE,
  COMMAND_VALIDATE,
  COMMAND_EXIT,
  COMMAND_GET_CHALLENGE,
  COMMAND_REVOKE_CHALLENGE,
  COMMAND_GET_USER_INFO,
  COMMAND_SET_FEATURE,
  COMMAND_CLR_FEATURE,
  COMMAND_RESET_LOCKOUT,
  COMMAND_MIGRATE,
  COMMAND_EXIT_DIRTY,
  COMMAND_VERIFY_HAT,
  COMMAND_COUNT,
  COMMAND_INTMAX = 0xffffffff
} FaceAuthInputCommands;
#define IS_IMAGE_PROCESSING_CMD(cmd) ((cmd) == COMMAND_ENROLL || (cmd) == COMMAND_VALIDATE || (cmd) == COMMAND_MIGRATE)
typedef enum _faceauth_ack_messages {
  STATUS_NONE = 0,
  STATUS_READY,
  STATUS_BUSY,
  STATUS_CLEAN,
  STATUS_INTMAX = 0xffffffff
} FaceAckMessages;
struct Coordinate {
  int32_t x;
  int32_t y;
} __attribute__((packed));
struct BoundingBox {
  struct Coordinate top_left;
  struct Coordinate bottom_right;
} __attribute__((packed));
struct BoundingBoxes {
  struct BoundingBox right_box;
  struct BoundingBox left_box;
} __attribute__((packed));
enum FaceLandmarks {
  LEFT_EYE = 0,
  RIGHT_EYE,
  NOSE_TIP,
  MOUTH_CENTER,
  LEFT_EAR_TRAGION,
  RIGHT_EAR_TRAGION,
  NUM_FACE_LANDMARKS,
  FACE_LANDMARK_INTMAX = 0xffffffff
};
struct Face {
  int32_t x1;
  int32_t y1;
  int32_t x2;
  int32_t y2;
  struct Coordinate landmark[NUM_FACE_LANDMARKS];
  uint32_t confidence;
  int32_t pan_angle;
  int32_t tilt_angle;
  int32_t roll_angle;
} __attribute__((packed));
struct SerialVerificationToken {
  uint64_t challenge;
  uint64_t timestamp;
  uint32_t security_level;
  uint8_t mac[32];
} __attribute__((packed));
#endif
