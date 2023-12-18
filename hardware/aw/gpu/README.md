
This confidential and proprietary software should be used under the licensing agreement from Allwinner Technology.

Copyright (C) 2018-2019 Allwinner Technology Limited All rights reserved.

Author: Albert Yu &lt;yuxyun@allwinnertech.com&gt;

The entire notice above must be reproduced on all authorised copies and copies may only be made to the extent permitted by a licensing agreement from Allwinner Technology Limited.

# Integrate GPU on Android
- Step 1: add TARGET_GPU_TYPE in device/sotfwinner/<platform>-common/BoardConfigCommon.mk, the TARGET_GPU_TYPE of different platforms can be found in Appendix A.
- Step 2: include hardware/aw/gpu/product_config.mk in device/sotfwinner/&lt;platform&gt;-common/BoardConfigCommon.mk.
- Step 3: configure opengles version property:
131072=opengles 2.0
196608=opengles 3.0
196609=opengles 3.1
196610=opengles 3.2
check the Appendix B to choose which version the gpu should be.

---Note---  
We designed product_config.mk to encapsulate all gpu related configuration items, making system engineers works more efficiently. We should include hardware/aw/gpu/product_config.mk in <platform> BoardConfigCommon.mk after TARGET_GPU_TYPE & TARGET_ARCH set.
Main work of product_config.mk:
-- Set PRODUCT_GPU_FILES for generating all gpu-copy-file pacakge
-- Set PRODUCT_GPU_PACKAGES to requested for gpu-package
-- Set PRODUCT_GPU_PROPERTIES for genarating gpu-property package

\------------

# Appendix A
platform | TARGET_GPU_TYPE
---|---
dolphin | mali400
venus | mali400
petrel | mali-t720
cupid | mali-g31
mercury | mali-g31
ceres | ge8300

# Appendix B
Highest OpenGLES Version | TARGET_GPU_TYPE
---|---
2.0 | mali400, mali450, sgx544
3.1 | mali-t720
3.2 | mali-t760, mali-g31, ge8300
