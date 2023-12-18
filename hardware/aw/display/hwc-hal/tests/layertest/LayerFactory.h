
#ifndef _LAYER_FACTORY_H_
#define _LAYER_FACTORY_H_

#include <memory>
#include <vector>

#include "Layer.h"
#include "private_handle.h"
#include "LayerPlannerTest.h"

using namespace sunxi;

struct LayerProperty {
    int PixelFormat;
    uint32_t Zorder;
    float Alpha;
    int32_t BlendMode;
    int32_t Composition;
    int32_t Dataspace;
    int32_t Transform;
    hwc_frect_t Crop;
    hwc_rect_t DisplayFrame;
};

class LayerFactory {
public:
    LayerFactory()
        : mBuffers(), mLayers() { }


    static void buffer_deleter (private_handle_t* ptr) {
        free(ptr);
    }

    std::shared_ptr<Layer> create(const LayerProperty& property)
    {
        std::shared_ptr<Layer> layer = std::make_shared<Layer>();
        layer->setLayerBlendMode(property.BlendMode);
        layer->setLayerCompositionType(property.Composition);
        layer->setLayerPlaneAlpha(property.Alpha);
        layer->setLayerZOrder(property.Zorder);
        layer->setLayerDataspace(property.Dataspace);
        layer->setLayerTransform(property.Transform);
        layer->setLayerSourceCrop(property.Crop);
        layer->setLayerDisplayFrame(property.DisplayFrame);

        if (property.PixelFormat != -1) {
            private_handle_t* handle = (private_handle_t *)malloc(sizeof(private_handle_t));
            std::shared_ptr<private_handle_t> buffer(handle, buffer_deleter);
            mBuffers.push_back(buffer);
            buffer->format = property.PixelFormat;
            layer->setLayerBuffer((buffer_handle_t)buffer.get(), -1);
        }

        mLayers.push_back(layer);
        return layer;
    }

    std::shared_ptr<Layer> getLayer(const LayerProperty& property)
    {
        for (auto& layer : mLayers) {
            if (layer->zOrder() == property.Zorder)
                return layer;
        }
        return nullptr;
    }

    void reset() {
        mBuffers.clear();
        mLayers.clear();
    }

private:
    std::vector<std::shared_ptr<private_handle_t>> mBuffers;
    std::vector<std::shared_ptr<Layer>> mLayers;
};

#endif

