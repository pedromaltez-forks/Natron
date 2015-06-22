//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "RotoPaint.h"

#include <sstream>

#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"

using namespace Natron;

std::string
RotoPaint::getDescription() const
{
    return "RotoPaint is a vector based free-hand drawing node that helps for tasks such as rotoscoping, matting, etc...\n";
}

RotoPaint::RotoPaint(boost::shared_ptr<Natron::Node> node)
: EffectInstance(node)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}


RotoPaint::~RotoPaint()
{
    
}

std::string
RotoPaint::getInputLabel (int inputNb) const
{
    if (inputNb == 10) {
        return "Mask";
    } else if (inputNb == 0) {
        return "Bg";
    } else {
        std::stringstream ss;
        ss << "Bg" << inputNb + 1;
        return ss.str();
    }
}

bool
RotoPaint::isInputMask(int inputNb) const
{
    return inputNb == 10;
}

void
RotoPaint::addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps)
{
    
    if (inputNb != 10) {
        comps->push_back(ImageComponents::getRGBAComponents());
        comps->push_back(ImageComponents::getRGBComponents());
        comps->push_back(ImageComponents::getXYComponents());
    }
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
RotoPaint::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

void
RotoPaint::initializeKnobs()
{
    
}


void
RotoPaint::getPreferredDepthAndComponents(int inputNb,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const
{

    if (inputNb != 10) {
        comp->push_back(ImageComponents::getRGBAComponents());
    } else {
        comp->push_back(ImageComponents::getAlphaComponents());
    }
    *depth = eImageBitDepthFloat;
}


Natron::ImagePremultiplicationEnum
RotoPaint::getOutputPremultiplication() const
{
  
//    EffectInstance* input = getInput(0);
//    if (input) {
//        return input->getOutputPremultiplication();
//    } else {
        return eImagePremultiplicationPremultiplied;
//    }
}

double
RotoPaint::getPreferredAspectRatio() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        return 1.;
    }
    
}

void
RotoPaint::onInputChanged(int inputNb)
{
    
    boost::shared_ptr<Node> inputNode = getNode()->getInput(0);
    getNode()->getRotoContext()->onRotoPaintInputChanged(inputNode);
    EffectInstance::onInputChanged(inputNb);
    
}

Natron::StatusEnum
RotoPaint::getRegionOfDefinition(U64 hash,SequenceTime time, const RenderScale & scale, int view, RectD* rod)
{
    (void)EffectInstance::getRegionOfDefinition(hash, time, scale, view, rod);
    
    RectD maskRod;
    getNode()->getRotoContext()->getMaskRegionOfDefinition(time, view, &maskRod);
    if (rod->isNull()) {
        *rod = maskRod;
    } else {
        rod->merge(maskRod);
    }
    return Natron::eStatusOK;
}

bool
RotoPaint::isIdentity(SequenceTime time,
                      const RenderScale & scale,
                      const RectI & roi,
                      int view,
                      SequenceTime* inputTime,
                      int* inputNb)
{
    boost::shared_ptr<Node> node = getNode();
    EffectInstance* maskInput = getInput(10);
    if (maskInput) {
        
        RectD maskRod;
        bool isProjectFormat;
        (void)maskInput->getRegionOfDefinition_public(maskInput->getRenderHash(), time, scale, view, &maskRod, &isProjectFormat);
        RectI maskPixelRod;
        maskRod.toPixelEnclosing(scale, getPreferredAspectRatio(), &maskPixelRod);
        if (!maskPixelRod.intersects(roi)) {
            *inputTime = time;
            *inputNb = 0;
            return true;
        }
    }
    
    std::list<boost::shared_ptr<RotoDrawableItem> > items = node->getRotoContext()->getCurvesByRenderOrder();
    if (items.empty()) {
        *inputNb = 0;
        *inputTime = time;
        return true;
    }
    return false;
}

class RotoPaintParallelArgsSetter
{
    NodeList _nodes;
public:
    
    RotoPaintParallelArgsSetter(const NodeList& nodes,
                                int time,
                                int view,
                                bool isRenderUserInteraction,
                                bool isSequential,
                                bool canAbort,
                                U64 renderAge,
                                Natron::OutputEffectInstance* renderRequester,
                                int textureIndex,
                                const TimeLine* timeline,
                                bool isAnalysis)
    : _nodes(nodes)
    {
        
        for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            Natron::EffectInstance* liveInstance = (*it)->getLiveInstance();
            assert(liveInstance);
            Natron::RenderSafetyEnum safety = liveInstance->renderThreadSafety();
            liveInstance->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, canAbort, (*it)->getHashValue(), (*it)->getRotoAge(), renderAge,renderRequester,textureIndex, timeline, isAnalysis, false, safety);
        }

    }
    
    ~RotoPaintParallelArgsSetter()
    {
        for (NodeList::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            (*it)->getLiveInstance()->invalidateParallelRenderArgsTLS();
        }
    }
};

Natron::StatusEnum
RotoPaint::render(const RenderActionArgs& args)
{
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    std::list<boost::shared_ptr<RotoDrawableItem> > items = roto->getCurvesByRenderOrder();
    
    std::list<ImageComponents> bgComps;
    Natron::ImageBitDepthEnum bgDepth;
    getPreferredDepthAndComponents(0, &bgComps, &bgDepth);
    assert(!bgComps.empty());

    
    std::list<ImageComponents> neededComps;
    for (std::list<std::pair<Natron::ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator plane = args.outputPlanes.begin();
         plane != args.outputPlanes.end(); ++plane) {
        neededComps.push_back(plane->first);
    }
    if (items.empty()) {
        
        RectI bgImgRoI;
        ImagePtr bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, bgComps.front(), bgDepth, getPreferredAspectRatio(), false, &bgImgRoI);
        
        for (std::list<std::pair<Natron::ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            
            if (bgImg) {
                plane->second->pasteFrom(*bgImg, args.roi, false);
            } else {
                plane->second->fillZero(args.roi);
            }
            
        }
    } else {
        
        
        NodeList rotoPaintNodes;
        roto->getRotoPaintTreeNodes(&rotoPaintNodes);
        
        const boost::shared_ptr<RotoDrawableItem>& firstStrokeItem = items.back();
        assert(firstStrokeItem);
        boost::shared_ptr<Node> bottomMerge = firstStrokeItem->getMergeNode();
        
        bool duringPaintStroke = bottomMerge->isDuringPaintStrokeCreation();
        
        boost::shared_ptr<RotoPaintParallelArgsSetter> frameArgs;
        if (!duringPaintStroke) {
            //In the other case this is handled by the viewer TLS
            frameArgs.reset(new RotoPaintParallelArgsSetter(rotoPaintNodes,
                                                            args.time,
                                                            args.view,
                                                            args.isRenderResponseToUserInteraction,
                                                            args.isSequentialRender,
                                                            false,
                                                            0, //render Age
                                                            0, // viewer requester
                                                            0, //texture index
                                                            getApp()->getTimeLine().get(),
                                                            false ));
        }
        
        
        
        RenderingFlagSetter flagIsRendering(bottomMerge.get());

        
        unsigned int mipMapLevel = Image::getLevelFromScale(args.mappedScale.x);
        RenderRoIArgs rotoPaintArgs(args.time,
                                    args.mappedScale,
                                    mipMapLevel,
                                    args.view,
                                    args.byPassCache,
                                    args.roi,
                                    RectD(),
                                    neededComps,
                                    bgDepth,
                                    this);
        ImageList rotoPaintImages;
        RenderRoIRetCode code = bottomMerge->getLiveInstance()->renderRoI(rotoPaintArgs, &rotoPaintImages);
        if (code == eRenderRoIRetCodeFailed) {
            return Natron::eStatusFailed;
        } else if (code == eRenderRoIRetCodeAborted) {
            return Natron::eStatusOK;
        } else if (rotoPaintImages.empty()) {
            for (std::list<std::pair<Natron::ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator plane = args.outputPlanes.begin();
                 plane != args.outputPlanes.end(); ++plane) {
                
                plane->second->fillZero(args.roi);
                
                
            }
            return Natron::eStatusOK;
        }
        assert(rotoPaintImages.size() == args.outputPlanes.size());
        
        ImageList::iterator rotoImagesIt = rotoPaintImages.begin();
        for (std::list<std::pair<Natron::ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane, ++rotoImagesIt) {
            if ((*rotoImagesIt)->getComponents() != plane->second->getComponents()) {
                
                (*rotoImagesIt)->convertToFormat(args.roi,
                                                 getApp()->getDefaultColorSpaceForBitDepth((*rotoImagesIt)->getBitDepth()),
                                                 getApp()->getDefaultColorSpaceForBitDepth(plane->second->getBitDepth()), 3
                                                 , false, false, plane->second.get());
            } else {
                plane->second->pasteFrom(**rotoImagesIt, args.roi, false);
            }
        }
    }
    
    return Natron::eStatusOK;
}

void
RotoPaint::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();
    NodeList rotoPaintNodes;
    boost::shared_ptr<RotoContext> roto = getNode()->getRotoContext();
    assert(roto);
    roto->getRotoPaintTreeNodes(&rotoPaintNodes);
    for (NodeList::iterator it = rotoPaintNodes.begin(); it!=rotoPaintNodes.end(); ++it) {
        (*it)->clearLastRenderedImage();
    }
}