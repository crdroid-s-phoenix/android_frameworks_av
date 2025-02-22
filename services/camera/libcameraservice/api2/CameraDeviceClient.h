/*
 * Copyright (C) 2013-2018 The Android Open Source Project
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

#ifndef ANDROID_SERVERS_CAMERA_PHOTOGRAPHY_CAMERADEVICECLIENT_H
#define ANDROID_SERVERS_CAMERA_PHOTOGRAPHY_CAMERADEVICECLIENT_H

#include <android/hardware/camera2/BnCameraDeviceUser.h>
#include <android/hardware/camera2/ICameraDeviceCallbacks.h>
#include <camera/camera2/OutputConfiguration.h>
#include <camera/camera2/SessionConfiguration.h>
#include <camera/camera2/SubmitInfo.h>

#include "CameraOfflineSessionClient.h"
#include "CameraService.h"
#include "common/FrameProcessorBase.h"
#include "common/Camera2ClientBase.h"
#include "CompositeStream.h"
#include "utils/SessionConfigurationUtils.h"

using android::camera3::OutputStreamInfo;
using android::camera3::CompositeStream;

namespace android {

struct CameraDeviceClientBase :
         public CameraService::BasicClient,
         public hardware::camera2::BnCameraDeviceUser
{
    typedef hardware::camera2::ICameraDeviceCallbacks TCamCallbacks;

    const sp<hardware::camera2::ICameraDeviceCallbacks>& getRemoteCallback() {
        return mRemoteCallback;
    }

protected:
    CameraDeviceClientBase(const sp<CameraService>& cameraService,
            const sp<hardware::camera2::ICameraDeviceCallbacks>& remoteCallback,
            const String16& clientPackageName,
            const std::optional<String16>& clientFeatureId,
            const String8& cameraId,
            int api1CameraId,
            int cameraFacing,
            int sensorOrientation,
            int clientPid,
            uid_t clientUid,
            int servicePid);

    sp<hardware::camera2::ICameraDeviceCallbacks> mRemoteCallback;
};

/**
 * Implements the binder ICameraDeviceUser API,
 * meant for HAL3-public implementation of
 * android.hardware.photography.CameraDevice
 */
class CameraDeviceClient :
        public Camera2ClientBase<CameraDeviceClientBase>,
        public camera2::FrameProcessorBase::FilteredListener
{
public:
    /**
     * ICameraDeviceUser interface (see ICameraDeviceUser for details)
     */

    // Note that the callee gets a copy of the metadata.
    virtual binder::Status submitRequest(
            const hardware::camera2::CaptureRequest& request,
            bool streaming = false,
            /*out*/
            hardware::camera2::utils::SubmitInfo *submitInfo = nullptr) override;
    // List of requests are copied.
    virtual binder::Status submitRequestList(
            const std::vector<hardware::camera2::CaptureRequest>& requests,
            bool streaming = false,
            /*out*/
            hardware::camera2::utils::SubmitInfo *submitInfo = nullptr) override;
    virtual binder::Status cancelRequest(int requestId,
            /*out*/
            int64_t* lastFrameNumber = NULL) override;

    virtual binder::Status beginConfigure() override;

    virtual binder::Status endConfigure(int operatingMode,
            const hardware::camera2::impl::CameraMetadataNative& sessionParams,
            int64_t startTimeMs,
            /*out*/
            std::vector<int>* offlineStreamIds) override;

    // Verify specific session configuration.
    virtual binder::Status isSessionConfigurationSupported(
            const SessionConfiguration& sessionConfiguration,
            /*out*/
            bool* streamStatus) override;

    // Returns -EBUSY if device is not idle or in error state
    virtual binder::Status deleteStream(int streamId) override;

    virtual binder::Status createStream(
            const hardware::camera2::params::OutputConfiguration &outputConfiguration,
            /*out*/
            int32_t* newStreamId = NULL) override;

    // Create an input stream of width, height, and format.
    virtual binder::Status createInputStream(int width, int height, int format,
            bool isMultiResolution,
            /*out*/
            int32_t* newStreamId = NULL) override;

    // Get the buffer producer of the input stream
    virtual binder::Status getInputSurface(
            /*out*/
            view::Surface *inputSurface) override;

    // Create a request object from a template.
    virtual binder::Status createDefaultRequest(int templateId,
            /*out*/
            hardware::camera2::impl::CameraMetadataNative* request) override;

    // Get the static metadata for the camera
    // -- Caller owns the newly allocated metadata
    virtual binder::Status getCameraInfo(
            /*out*/
            hardware::camera2::impl::CameraMetadataNative* cameraCharacteristics) override;

    // Wait until all the submitted requests have finished processing
    virtual binder::Status waitUntilIdle() override;

    // Flush all active and pending requests as fast as possible
    virtual binder::Status flush(
            /*out*/
            int64_t* lastFrameNumber = NULL) override;

    // Prepare stream by preallocating its buffers
    virtual binder::Status prepare(int32_t streamId) override;

    // Tear down stream resources by freeing its unused buffers
    virtual binder::Status tearDown(int32_t streamId) override;

    // Prepare stream by preallocating up to maxCount of its buffers
    virtual binder::Status prepare2(int32_t maxCount, int32_t streamId) override;

    // Update an output configuration
    virtual binder::Status updateOutputConfiguration(int streamId,
            const hardware::camera2::params::OutputConfiguration &outputConfiguration) override;

    // Finalize the output configurations with surfaces not added before.
    virtual binder::Status finalizeOutputConfigurations(int32_t streamId,
            const hardware::camera2::params::OutputConfiguration &outputConfiguration) override;

    virtual binder::Status setCameraAudioRestriction(int32_t mode) override;

    virtual binder::Status getGlobalAudioRestriction(/*out*/int32_t* outMode) override;

    virtual binder::Status switchToOffline(
            const sp<hardware::camera2::ICameraDeviceCallbacks>& cameraCb,
            const std::vector<int>& offlineOutputIds,
            /*out*/
            sp<hardware::camera2::ICameraOfflineSession>* session) override;

    /**
     * Interface used by CameraService
     */

    CameraDeviceClient(const sp<CameraService>& cameraService,
            const sp<hardware::camera2::ICameraDeviceCallbacks>& remoteCallback,
            const String16& clientPackageName,
            const std::optional<String16>& clientFeatureId,
            const String8& cameraId,
            int cameraFacing,
            int sensorOrientation,
            int clientPid,
            uid_t clientUid,
            int servicePid,
            bool overrideForPerfClass);
    virtual ~CameraDeviceClient();

    virtual status_t      initialize(sp<CameraProviderManager> manager,
            const String8& monitorTags) override;

    virtual status_t      setRotateAndCropOverride(uint8_t rotateAndCrop) override;

    virtual bool          supportsCameraMute();
    virtual status_t      setCameraMute(bool enabled);

    virtual status_t      dump(int fd, const Vector<String16>& args);

    virtual status_t      dumpClient(int fd, const Vector<String16>& args);

    /**
     * Device listener interface
     */

    virtual void notifyIdle(int64_t requestCount, int64_t resultErrorCount, bool deviceError,
                            const std::vector<hardware::CameraStreamStats>& streamStats);
    virtual void notifyError(int32_t errorCode,
                             const CaptureResultExtras& resultExtras);
    virtual void notifyShutter(const CaptureResultExtras& resultExtras, nsecs_t timestamp);
    virtual void notifyPrepared(int streamId);
    virtual void notifyRequestQueueEmpty();
    virtual void notifyRepeatingRequestError(long lastFrameNumber);

    void setImageDumpMask(int mask) { if (mDevice != nullptr) mDevice->setImageDumpMask(mask); }
    /**
     * Interface used by independent components of CameraDeviceClient.
     */
protected:
    /** FilteredListener implementation **/
    virtual void          onResultAvailable(const CaptureResult& result);
    virtual void          detachDevice();

    // Calculate the ANativeWindow transform from android.sensor.orientation
    status_t              getRotationTransformLocked(/*out*/int32_t* transform);

    bool isUltraHighResolutionSensor(const String8 &cameraId);

    bool isSensorPixelModeConsistent(const std::list<int> &streamIdList,
            const CameraMetadata &settings);

    const CameraMetadata &getStaticInfo(const String8 &cameraId);

private:
    // StreamSurfaceId encapsulates streamId + surfaceId for a particular surface.
    // streamId specifies the index of the stream the surface belongs to, and the
    // surfaceId specifies the index of the surface within the stream. (one stream
    // could contain multiple surfaces.)
    class StreamSurfaceId final {
    public:
        StreamSurfaceId() {
            mStreamId = -1;
            mSurfaceId = -1;
        }
        StreamSurfaceId(int32_t streamId, int32_t surfaceId) {
            mStreamId = streamId;
            mSurfaceId = surfaceId;
        }
        int32_t streamId() const {
            return mStreamId;
        }
        int32_t surfaceId() const {
            return mSurfaceId;
        }

    private:
        int32_t mStreamId;
        int32_t mSurfaceId;

    }; // class StreamSurfaceId

private:
    /** ICameraDeviceUser interface-related private members */

    /** Preview callback related members */
    sp<camera2::FrameProcessorBase> mFrameProcessor;

    std::vector<int32_t> mSupportedPhysicalRequestKeys;

    template<typename TProviderPtr>
    status_t      initializeImpl(TProviderPtr providerPtr, const String8& monitorTags);

    /** Utility members */
    binder::Status checkPidStatus(const char* checkLocation);
    bool enforceRequestPermissions(CameraMetadata& metadata);

    // Create an output stream with surface deferred for future.
    binder::Status createDeferredSurfaceStreamLocked(
            const hardware::camera2::params::OutputConfiguration &outputConfiguration,
            bool isShared,
            int* newStreamId = NULL);

    // Set the stream transform flags to automatically rotate the camera stream for preview use
    // cases.
    binder::Status setStreamTransformLocked(int streamId);

    // Utility method to insert the surface into SurfaceMap
    binder::Status insertGbpLocked(const sp<IGraphicBufferProducer>& gbp,
            /*out*/SurfaceMap* surfaceMap, /*out*/Vector<int32_t>* streamIds,
            /*out*/int32_t*  currentStreamId);

    // Utility method that maps AIDL request templates.
    binder::Status mapRequestTemplate(int templateId,
            camera_request_template_t* tempId /*out*/);

    // IGraphicsBufferProducer binder -> Stream ID + Surface ID for output streams
    KeyedVector<sp<IBinder>, StreamSurfaceId> mStreamMap;

    // Stream ID -> OutputConfiguration. Used for looking up Surface by stream/surface index
    KeyedVector<int32_t, hardware::camera2::params::OutputConfiguration> mConfiguredOutputs;

    struct InputStreamConfiguration {
        bool configured;
        int32_t width;
        int32_t height;
        int32_t format;
        int32_t id;
    } mInputStream;

    // Streaming request ID
    int32_t mStreamingRequestId;
    Mutex mStreamingRequestIdLock;
    static const int32_t REQUEST_ID_NONE = -1;

    int32_t mRequestIdCounter;
    bool mPrivilegedClient;

    std::vector<std::string> mPhysicalCameraIds;

    // The list of output streams whose surfaces are deferred. We have to track them separately
    // as there are no surfaces available and can not be put into mStreamMap. Once the deferred
    // Surface is configured, the stream id will be moved to mStreamMap.
    Vector<int32_t> mDeferredStreams;

    // stream ID -> outputStreamInfo mapping
    std::unordered_map<int32_t, OutputStreamInfo> mStreamInfoMap;

    // map high resolution camera id (logical / physical) -> list of stream ids configured
    std::unordered_map<std::string, std::unordered_set<int>> mHighResolutionCameraIdToStreamIdSet;

    // set of high resolution camera id (logical / physical)
    std::unordered_set<std::string> mHighResolutionSensors;

    // Synchronize access to 'mCompositeStreamMap'
    Mutex mCompositeLock;
    KeyedVector<sp<IBinder>, sp<CompositeStream>> mCompositeStreamMap;

    sp<CameraProviderManager> mProviderManager;

    // Override the camera characteristics for performance class primary cameras.
    bool mOverrideForPerfClass;
};

}; // namespace android

#endif
