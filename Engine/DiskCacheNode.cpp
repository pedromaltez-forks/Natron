//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "DiskCacheNode.h"

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"

using namespace Natron;

struct DiskCacheNodePrivate
{
    boost::weak_ptr<Choice_Knob> frameRange;
    boost::weak_ptr<Int_Knob> firstFrame;
    boost::weak_ptr<Int_Knob> lastFrame;
    boost::weak_ptr<Button_Knob> preRender;
    
    DiskCacheNodePrivate()
    {
        
    }
};

DiskCacheNode::DiskCacheNode(boost::shared_ptr<Node> node)
: OutputEffectInstance(node)
, _imp(new DiskCacheNodePrivate())
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}


void
DiskCacheNode::addAcceptedComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}
void
DiskCacheNode::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

bool
DiskCacheNode::shouldCacheOutput(bool /*isFrameVaryingOrAnimated*/) const
{
    return true;
}

void
DiskCacheNode::initializeKnobs()
{
    boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(this, "Controls");
    
    boost::shared_ptr<Choice_Knob> frameRange = Natron::createKnob<Choice_Knob>(this, "Frame range");
    frameRange->setName("frameRange");
    frameRange->setAnimationEnabled(false);
    std::vector<std::string> choices;
    choices.push_back("Input frame range");
    choices.push_back("Project frame range");
    choices.push_back("Manual");
    frameRange->populateChoices(choices);
    frameRange->setEvaluateOnChange(false);
    frameRange->setDefaultValue(0);
    page->addKnob(frameRange);
    _imp->frameRange = frameRange;
    
    boost::shared_ptr<Int_Knob> firstFrame = Natron::createKnob<Int_Knob>(this, "First frame");
    firstFrame->setAnimationEnabled(false);
    firstFrame->setName("firstFrame");
    firstFrame->disableSlider();
    firstFrame->setEvaluateOnChange(false);
    firstFrame->setAddNewLine(false);
    firstFrame->setDefaultValue(1);
    firstFrame->setSecret(true);
    page->addKnob(firstFrame);
    _imp->firstFrame = firstFrame;
    
    boost::shared_ptr<Int_Knob> lastFrame = Natron::createKnob<Int_Knob>(this, "Last frame");
    lastFrame->setAnimationEnabled(false);
    lastFrame->setName("LastFrame");
    lastFrame->disableSlider();
    lastFrame->setEvaluateOnChange(false);
    lastFrame->setDefaultValue(100);
    lastFrame->setSecret(true);
    page->addKnob(lastFrame);
    _imp->lastFrame = lastFrame;
    
    boost::shared_ptr<Button_Knob> preRender = Natron::createKnob<Button_Knob>(this, "Pre-cache");
    preRender->setName("preRender");
    preRender->setEvaluateOnChange(false);
    preRender->setHintToolTip("Cache the frame range specified by rendering images at zoom-level 100% only.");
    page->addKnob(preRender);
    _imp->preRender = preRender;
    
    
}

void
DiskCacheNode::knobChanged(KnobI* k, Natron::ValueChangedReasonEnum /*reason*/, int /*view*/, SequenceTime /*time*/,
                           bool /*originatedFromMainThread*/)
{
    if (_imp->frameRange.lock().get() == k) {
        int idx = _imp->frameRange.lock()->getValue();
        switch (idx) {
            case 0:
            case 1:
                _imp->firstFrame.lock()->setSecret(true);
                _imp->lastFrame.lock()->setSecret(true);
                break;
            case 2:
                _imp->firstFrame.lock()->setSecret(false);
                _imp->lastFrame.lock()->setSecret(false);
                break;
            default:
                break;
        }
    } else if (_imp->preRender.lock().get() == k) {
        AppInstance::RenderWork w;
        w.writer = this;
        w.firstFrame = INT_MIN;
        w.lastFrame = INT_MAX;
        std::list<AppInstance::RenderWork> works;
        works.push_back(w);
        getApp()->startWritersRendering(works);
    }
}

void
DiskCacheNode::getFrameRange(SequenceTime *first,SequenceTime *last)
{
    int idx = _imp->frameRange.lock()->getValue();
    switch (idx) {
        case 0: {
            EffectInstance* input = getInput(0);
            if (input) {
                input->getFrameRange_public(input->getHash(), first, last);
            }
        } break;
        case 1: {
            getApp()->getFrameRange(first, last);
        } break;
        case 2: {
            *first = _imp->firstFrame.lock()->getValue();
            *last = _imp->lastFrame.lock()->getValue();
        };
        default:
            break;
    }
}

void
DiskCacheNode::getPreferredDepthAndComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredDepthAndComponents(-1, comp, depth);
    } else {
        comp->push_back(ImageComponents::getRGBAComponents());
        *depth = eImageBitDepthFloat;
    }
}


Natron::ImagePremultiplicationEnum
DiskCacheNode::getOutputPremultiplication() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getOutputPremultiplication();
    } else {
        return eImagePremultiplicationPremultiplied;
    }

}

double
DiskCacheNode::getPreferredAspectRatio() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        return 1.;
    }

}

Natron::StatusEnum
DiskCacheNode::render(const RenderActionArgs& args)
{
    
    assert(args.outputPlanes.size() == 1);
    
    EffectInstance* input = getInput(0);
    if (!input) {
        return eStatusFailed;
    }
    
    ImageBitDepthEnum bitdepth;
    std::list<ImageComponents> components;
    input->getPreferredDepthAndComponents(-1, &components, &bitdepth);
    double par = input->getPreferredAspectRatio();
    
    const std::pair<ImageComponents,ImagePtr>& output = args.outputPlanes.front();
    
    for (std::list<ImageComponents> ::const_iterator it =components.begin(); it != components.end(); ++it) {
        RectI roiPixel;
        
        ImagePtr srcImg = getImage(0, args.time, args.originalScale, args.view, NULL, *it, bitdepth, par, false, &roiPixel);
        
        if (srcImg->getMipMapLevel() != output.second->getMipMapLevel()) {
            throw std::runtime_error("Host gave image with wrong scale");
        }
        if (srcImg->getComponents() != output.second->getComponents() || srcImg->getBitDepth() != output.second->getBitDepth()) {
            
            
            srcImg->convertToFormat(args.roi, getApp()->getDefaultColorSpaceForBitDepth( srcImg->getBitDepth() ),
                                    getApp()->getDefaultColorSpaceForBitDepth(output.second->getBitDepth()), 3, true, false, output.second.get());
        } else {
            output.second->pasteFrom(*srcImg, args.roi, output.second->usesBitMap() && srcImg->usesBitMap());
        }

    }
    
    return eStatusOK;
}

