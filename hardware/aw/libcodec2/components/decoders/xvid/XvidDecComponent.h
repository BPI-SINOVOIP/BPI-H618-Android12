/*
 * Copyright (C) 2021 by Allwinnertech Co. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_COMPONENTS_DECODERS_XVID_XVIDDECCOMPONENT_H_
#define ANDROID_COMPONENTS_DECODERS_XVID_XVIDDECCOMPONENT_H_

#include <sys/time.h>

#include <atomic>
#include <memory>

#include "HwC2Component.h"

namespace android {

class XvidDecComponent : public HwC2Component {
 public:
  class IntfImpl;
  std::shared_ptr<IntfImpl> mIntf;

  XvidDecComponent(const char *name, c2_node_id_t id,
                   const std::shared_ptr<IntfImpl> &intfImpl);
  virtual ~XvidDecComponent();

  /**
   * Initialize internal states of the component according to the config set
   * in the interface.
   *
   * This method is called during start(), but only at the first invocation or
   * after reset().
   */
  c2_status_t onInit() override;

  /**
   * Start the component.
   */
  c2_status_t onStart() override;

  /**
   * Stop the component.
   */
  c2_status_t onStop() override;

  /**
   * Reset the component.
   */
  void onReset() override;

  /**
   * Release the component.
   */
  void onRelease() override;

  /**
   * Flush the component.
   */
  c2_status_t onFlush_sm() override;

  /**
   * Process the given work and finish pending work using finish().
   *
   * \param[in,out]   work    the work to process
   * \param[in]       pool    the pool to use for allocating output blocks.
   */
  void process(const std::unique_ptr<C2Work> &work,
               const std::shared_ptr<C2BlockPool> &pool) override;

  /**
   * Drain the component and finish pending work using finish().
   *
   * \param[in]   drainMode   mode of drain.
   * \param[in]   pool        the pool to use for allocating output blocks.
   *
   * \retval C2_OK            The component has drained all pending output
   *                          work.
   * \retval C2_OMITTED       Unsupported mode (e.g. DRAIN_CHAIN)
   */
  c2_status_t drain(uint32_t drainMode,
                    const std::shared_ptr<C2BlockPool> &pool) override;

  void handleWorkCb2(std::unique_ptr<C2Work> work);

 private:
  std::unique_ptr<VdecComponent> mCodec;
  C2_DO_NOT_COPY(XvidDecComponent);
};

}  // namespace android

#endif  // ANDROID_COMPONENTS_DECODERS_XVID_XVIDDECCOMPONENT_H_
