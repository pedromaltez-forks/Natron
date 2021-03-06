//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "OfxHost.h"

#include <cassert>
#include <fstream>
#include <new> // std::bad_alloc
#include <stdexcept> // std::exception
#include <cctype> // tolower
#include <algorithm> // transform
#include <string>
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QThreadPool>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated-register)
#ifdef OFX_SUPPORTS_MULTITHREAD
#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtConcurrentMap>
#include <boost/bind.hpp>
#endif

//ofx
#include <ofxParametricParam.h>
#ifdef OFX_EXTENSIONS_NUKE
#include <nuke/fnOfxExtensions.h>
#endif
#include <ofxNatron.h>

#include "Global/Macros.h"
//ofx host support
#include <ofxhPluginAPICache.h>
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>
#include <ofxhParam.h>

#include <tuttle/ofxReadWrite.h>

//our version of parametric param suite support
#include "ofxhParametricParam.h"

#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"

#include "Engine/AppManager.h"
#include "Engine/OfxMemory.h"
#include "Engine/LibraryBinary.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Plugin.h"
#include "Engine/StandardPaths.h"
#include "Engine/Settings.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"
#include "Engine/Project.h"

using namespace Natron;


Natron::OfxHost::OfxHost()
    : _imageEffectPluginCache( new OFX::Host::ImageEffect::PluginCache(*this) )
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
    , _pluginsMutexes()
    , _pluginsMutexesLock(new QMutex)
#endif
{
}

Natron::OfxHost::~OfxHost()
{
    //Clean up, to be polite.
    OFX::Host::PluginCache::clearPluginCache();

    delete _imageEffectPluginCache;
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
    delete _pluginsMutexesLock;
#endif
}

void
Natron::OfxHost::setProperties()
{
    /* Known OpenFX host names:
       uk.co.thefoundry.nuke
       com.eyeonline.Fusion
       com.sonycreativesoftware.vegas
       Autodesk Toxik
       Assimilator
       Dustbuster
       DaVinciResolve
       DaVinciResolveLite
       Mistika
       com.apple.shake
       Baselight
       IRIDAS Framecycler
       com.chinadigitalvideo.dx
       Ramen
       TuttleOfx
       fr.inria.Natron
     
     Other possible names:
     Nuke
     Autodesk Toxik Render Utility
     Autodesk Toxik Python Bindings
     Toxik
     Fusion
     film master
     film cutter
     data conform
     nucoda
     phoenix
     Film Master
     Baselight
     Scratch
     DS OFX Host
     Avid DS
     Vegas
     CDV DX
     Resolve

     */
    _properties.setStringProperty( kOfxPropName,appPTR->getCurrentSettings()->getHostName() );
    _properties.setStringProperty(kOfxPropLabel, NATRON_APPLICATION_NAME); // "nuke" //< use this to pass for nuke
    _properties.setIntProperty(kOfxPropAPIVersion, 1, 0);  //OpenFX API v1.3
    _properties.setIntProperty(kOfxPropAPIVersion, 3, 1);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_MAJOR, 0);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_MINOR, 1);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_REVISION, 2);
    _properties.setStringProperty(kOfxPropVersionLabel, NATRON_VERSION_STRING);
    _properties.setIntProperty(kOfxImageEffectHostPropIsBackground, (int)appPTR->isBackground());
    _properties.setIntProperty(kOfxImageEffectPropSupportsOverlays, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultiResolution, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsTiles, 1);
    _properties.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGBA, 0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentAlpha, 1);
    if (appPTR->getCurrentSettings()->areRGBPixelComponentsSupported()) {
        _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGB, 2);
    }
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kFnOfxImageComponentMotionVectors, 3);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kFnOfxImageComponentStereoDisparity, 4);
    
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGenerator, 0 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextFilter, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGeneral, 2 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextTransition, 3 );
    
    ///Setting these makes The Foundry Furnace plug-ins fail in the load action
    //_properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextReader, 4 );
    //_properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextWriter, 5 );

    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthFloat,0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthShort,1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthByte,2);

    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipPARs, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFrameRate, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFielding, 0);
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomInteract, 1 );
    _properties.setIntProperty( kOfxParamHostPropSupportsStringAnimation, String_Knob::canAnimateStatic() );
    _properties.setIntProperty( kOfxParamHostPropSupportsChoiceAnimation, Choice_Knob::canAnimateStatic() );
    _properties.setIntProperty( kOfxParamHostPropSupportsBooleanAnimation, Bool_Knob::canAnimateStatic() );
    _properties.setIntProperty( kOfxParamHostPropSupportsCustomAnimation, String_Knob::canAnimateStatic() );
    _properties.setIntProperty(kOfxParamHostPropMaxParameters, -1);
    _properties.setIntProperty(kOfxParamHostPropMaxPages, 0);
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 0 );
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 1 );
    _properties.setIntProperty(kOfxImageEffectInstancePropSequentialRender, 2);
    _properties.setIntProperty(kOfxParamHostPropSupportsParametricAnimation, 0);
#ifdef OFX_EXTENSIONS_NUKE
    ///Nuke transform suite
    _properties.setIntProperty(kFnOfxImageEffectCanTransform, 1);
    
    ///Plane suite
    _properties.setIntProperty(kFnOfxImageEffectPropMultiPlanar, 1);
#endif
#ifdef OFX_EXTENSIONS_NATRON
    ///Natron extensions
    _properties.setIntProperty(kNatronOfxHostIsNatron, 1);
    _properties.setIntProperty(kNatronOfxParamHostPropSupportsDynamicChoices, 1);
    _properties.setIntProperty(kNatronOfxParamPropChoiceCascading, 1);
    _properties.setStringProperty(kNatronOfxImageEffectPropChannelSelector, kOfxImageComponentRGBA);
    _properties.setIntProperty(kNatronOfxImageEffectPropHostMasking, 1);
    _properties.setIntProperty(kNatronOfxImageEffectPropHostMixing, 1);
#endif
    
}

OFX::Host::ImageEffect::Instance*
Natron::OfxHost::newInstance(void*,
                             OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                             OFX::Host::ImageEffect::Descriptor & desc,
                             const std::string & context)
{
    assert(plugin);


    return new Natron::OfxImageEffectInstance(plugin,desc,context,false);
}

/// Override this to create a descriptor, this makes the 'root' descriptor
OFX::Host::ImageEffect::Descriptor *
Natron::OfxHost::makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OfxImageEffectDescriptor(plugin);

    return desc;
}

/// used to construct a context description, rootContext is the main context
OFX::Host::ImageEffect::Descriptor *
Natron::OfxHost::makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OfxImageEffectDescriptor(rootContext, plugin);

    return desc;
}

/// used to construct populate the cache
OFX::Host::ImageEffect::Descriptor *
Natron::OfxHost::makeDescriptor(const std::string &bundlePath,
                                OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OfxImageEffectDescriptor(bundlePath, plugin);

    return desc;
}

/// message
OfxStatus
Natron::OfxHost::vmessage(const char* msgtype,
                          const char* /*id*/,
                          const char* format,
                          va_list args)
{
    assert(msgtype);
    assert(format);
    char buf[10000];
    sprintf(buf, format,args);
    std::string message(buf);
    std::string type(msgtype);

    if (type == kOfxMessageLog) {
        appPTR->writeToOfxLog_mt_safe( message.c_str() );
    } else if ( (type == kOfxMessageFatal) || (type == kOfxMessageError) ) {
        ///It seems that the only errors or warning that passes here are exceptions thrown by plug-ins
        ///(mainly Sapphire) while aborting a render. Instead of spamming the user of meaningless dialogs,
        ///just write to the log instead.
        //Natron::errorDialog(NATRON_APPLICATION_NAME, message);
        appPTR->writeToOfxLog_mt_safe( message.c_str() );
    } else if (type == kOfxMessageWarning) {
        ///It seems that the only errors or warning that passes here are exceptions thrown by plug-ins
        ///(mainly Sapphire) while aborting a render. Instead of spamming the user of meaningless dialogs,
        ///just write to the log instead.
        //        Natron::warningDialog(NATRON_APPLICATION_NAME, message);
        appPTR->writeToOfxLog_mt_safe( message.c_str() );
    } else if (type == kOfxMessageMessage) {
        Natron::informationDialog(NATRON_APPLICATION_NAME, message);
    } else if (type == kOfxMessageQuestion) {
        if (Natron::questionDialog(NATRON_APPLICATION_NAME, message, false) == Natron::eStandardButtonYes) {
            return kOfxStatReplyYes;
        } else {
            return kOfxStatReplyNo;
        }
    }

    return kOfxStatReplyDefault;
}

OfxStatus
Natron::OfxHost::setPersistentMessage(const char* type,
                                      const char* id,
                                      const char* format,
                                      va_list args)
{
    vmessage(type,id,format,args);

    return kOfxStatOK;
}

/// clearPersistentMessage
OfxStatus
Natron::OfxHost::clearPersistentMessage()
{
    return kOfxStatOK;
}

static std::string getContext_internal(const std::set<std::string> & contexts)
{
    std::string context;
    if (contexts.size() == 0) {
        throw std::runtime_error( std::string("Error: Plug-in does not support any context") );
        //context = kOfxImageEffectContextGeneral;
        //plugin->addContext(kOfxImageEffectContextGeneral);
    } else if (contexts.size() == 1) {
        context = ( *contexts.begin() );
        return context;
    } else {
        std::set<std::string>::iterator found = contexts.find(kOfxImageEffectContextReader);
        bool reader = found != contexts.end();
        if (reader) {
            context = kOfxImageEffectContextReader;
            return context;
        }
        
        found = contexts.find(kOfxImageEffectContextWriter);
        bool writer = found != contexts.end();
        if (writer) {
            context = kOfxImageEffectContextWriter;
            
            return context;
        }
        
        found = contexts.find(kNatronOfxImageEffectContextTracker);
        bool tracker = found != contexts.end();
        if (tracker) {
            context = kNatronOfxImageEffectContextTracker;
            
            return context;
        }
        
        
        
        found = contexts.find(kOfxImageEffectContextGeneral);
        bool general = found != contexts.end();
        if (general) {
            context = kOfxImageEffectContextGeneral;
            
            return context;
        }
        
        found = contexts.find(kOfxImageEffectContextFilter);
        bool filter = found != contexts.end();
        if (filter) {
            context = kOfxImageEffectContextFilter;
            
            return context;
        }
        
        found = contexts.find(kOfxImageEffectContextPaint);
        bool paint = found != contexts.end();
        if (paint) {
            context = kOfxImageEffectContextPaint;
            
            return context;
        }
        
        found = contexts.find(kOfxImageEffectContextGenerator);
        bool generator = found != contexts.end();
        if (generator) {
            context = kOfxImageEffectContextGenerator;
            
            return context;
        }
        
        found = contexts.find(kOfxImageEffectContextTransition);
        bool transition = found != contexts.end();
        if (transition) {
            context = kOfxImageEffectContextTransition;
            
            return context;
        }
    }
    return context;

}

OFX::Host::ImageEffect::Descriptor*
Natron::OfxHost::getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                             Natron::ContextEnum* ctx)
{
    OFX::Host::PluginHandle *pluginHandle;
    // getPluginHandle() must be called before getContexts():
    // it calls kOfxActionLoad on the plugin, which may set properties (including supported contexts)
    try {
        pluginHandle = plugin->getPluginHandle();
    } catch (...) {
        throw std::runtime_error(std::string("Error: Description failed while loading ") + plugin->getIdentifier());
    }
    
    if (!pluginHandle) {
        throw std::runtime_error(std::string("Error: Description failed while loading ") + plugin->getIdentifier());
    }
    assert(pluginHandle->getOfxPlugin() && pluginHandle->getOfxPlugin()->mainEntry);
    
    const std::set<std::string> & contexts = plugin->getContexts();
    
    std::string context = getContext_internal(contexts);
    if (context.empty()) {
        throw std::invalid_argument(QObject::tr("OpenFX plug-in has no valid context.").toStdString());
    }
    
    OFX::Host::PluginHandle* ph = plugin->getPluginHandle();
    assert( ph->getOfxPlugin() );
    assert(ph->getOfxPlugin()->mainEntry);
    (void)ph;
    OFX::Host::ImageEffect::Descriptor* desc = NULL;
    desc = plugin->getContext(context);
    if (!desc) {
        throw std::runtime_error(std::string("Failed to get description for OFX plugin in context ") + context);
    }
    
    //Create the mask clip if needed
    if (desc->isHostMaskingEnabled()) {
        const std::map<std::string,OFX::Host::ImageEffect::ClipDescriptor*>& clips = desc->getClips();
        std::map<std::string,OFX::Host::ImageEffect::ClipDescriptor*>::const_iterator found = clips.find("Mask");
        if (found == clips.end()) {
            OFX::Host::ImageEffect::ClipDescriptor* clip = desc->defineClip("Mask");
            OFX::Host::Property::Set& props = clip->getProps();
            props.setIntProperty(kOfxImageClipPropIsMask, 1);
            props.setStringProperty(kOfxImageEffectPropSupportedComponents, kOfxImageComponentAlpha, 0);
            if (context == kOfxImageEffectContextGeneral) {
                props.setIntProperty(kOfxImageClipPropOptional, 1);
            }
            props.setIntProperty(kOfxImageEffectPropSupportsTiles, desc->getProps().getIntProperty(kOfxImageEffectPropSupportsTiles) != 0);
            props.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 0);
        }
    }

    
    *ctx = OfxEffectInstance::mapToContextEnum(context);
    return desc;
}

boost::shared_ptr<AbstractOfxEffectInstance>
Natron::OfxHost::createOfxEffect(boost::shared_ptr<Natron::Node> node,
                                 const NodeSerialization* serialization,
                                 const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                                 bool allowFileDialogs,
                                 bool disableRenderScaleSupport)
{
    assert(node);
    const Natron::Plugin* natronPlugin = node->getPlugin();
    assert(natronPlugin);
    ContextEnum ctx;
    OFX::Host::ImageEffect::Descriptor* desc = natronPlugin->getOfxDesc(&ctx);
    OFX::Host::ImageEffect::ImageEffectPlugin* plugin = natronPlugin->getOfxPlugin();
    assert(plugin && desc && ctx != eContextNone);
    

    boost::shared_ptr<AbstractOfxEffectInstance> hostSideEffect(new OfxEffectInstance(node));
    if ( node && !node->getLiveInstance() ) {
        node->setLiveInstance(hostSideEffect);
    }

    hostSideEffect->createOfxImageEffectInstance(plugin, desc, ctx,serialization,paramValues,allowFileDialogs,disableRenderScaleSupport);

    return hostSideEffect;
}

void
Natron::OfxHost::addPathToLoadOFXPlugins(const std::string path)
{
    OFX::Host::PluginCache::getPluginCache()->addFileToPath(path);
}

#if defined(WINDOWS)
// defined in ofxhPluginCache.cpp
const TCHAR * getStdOFXPluginPath(const std::string &hostId);
#endif

void
Natron::OfxHost::loadOFXPlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* readersMap,
                                std::map<std::string,std::vector< std::pair<std::string,double> > >* writersMap)
{
    assert( OFX::Host::PluginCache::getPluginCache() );
    /// set the version label in the global cache
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion(NATRON_APPLICATION_NAME "OFXCachev1");

    /// register the image effect cache with the global plugin cache
    _imageEffectPluginCache->registerInCache( *OFX::Host::PluginCache::getPluginCache() );


#if defined(WINDOWS)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath( getStdOFXPluginPath("Nuke") );
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("C:\\Program Files\\Common Files\\OFX\\Nuke");
#endif
#if defined(__linux__) || defined(__FreeBSD__)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("/usr/OFX/Nuke");
#endif
#if defined(__APPLE__)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("/Library/OFX/Nuke");
#endif

    std::list<std::string> extraPluginsSearchPaths;
    appPTR->getCurrentSettings()->getOpenFXPluginsSearchPaths(&extraPluginsSearchPaths);
    for (std::list<std::string>::iterator it = extraPluginsSearchPaths.begin(); it != extraPluginsSearchPaths.end(); ++it) {
        if ( !(*it).empty() ) {
            OFX::Host::PluginCache::getPluginCache()->addFileToPath(*it);
        }
    }

    QDir dir( QCoreApplication::applicationDirPath() );
    dir.cdUp();
    std::string natronBundledPluginsPath = QString(dir.absolutePath() +  "/Plugins").toStdString();
    try {
        if ( appPTR->getCurrentSettings()->loadBundledPlugins() ) {
            if ( appPTR->getCurrentSettings()->preferBundledPlugins() ) {
                OFX::Host::PluginCache::getPluginCache()->prependFileToPath(natronBundledPluginsPath);
            } else {
                OFX::Host::PluginCache::getPluginCache()->addFileToPath(natronBundledPluginsPath);
            }
        }
    } catch (std::logic_error) {
        // ignore
    }

    /// now read an old cache
    // The cache location depends on the OS.
    // On OSX, it will be ~/Library/Caches/<organization>/<application>/OFXCache.xml
    //on Linux ~/.cache/<organization>/<application>/OFXCache.xml
    //on windows:
    QString ofxcachename = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationCache) + QDir::separator() + "OFXCache.xml";
    std::ifstream ifs( ofxcachename.toStdString().c_str() );
    if ( ifs.is_open() ) {
        OFX::Host::PluginCache::getPluginCache()->readCache(ifs);
        ifs.close();
    }
    OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();

    // write the cache NOW (it won't change anyway)
    /// flush out the current cache
    writeOFXCache();

    /*Filling node name list and plugin grouping*/
    typedef std::map<OFX::Host::ImageEffect::MajorPlugin,OFX::Host::ImageEffect::ImageEffectPlugin *> PMap;
    const PMap& ofxPlugins =
    _imageEffectPluginCache->getPluginsByIDMajor();
    
    
    for (PMap::const_iterator it = ofxPlugins.begin();
         it != ofxPlugins.end(); ++it) {
        OFX::Host::ImageEffect::ImageEffectPlugin* p = it->second;
        assert(p);
        if (p->getContexts().size() == 0) {
            continue;
        }

        std::string openfxId = p->getIdentifier();
        const std::string & grouping = p->getDescriptor().getPluginGrouping();
        const std::string & bundlePath = p->getBinary()->getBundlePath();
        std::string pluginLabel = OfxEffectInstance::makePluginLabel( p->getDescriptor().getShortLabel(),
                                                                      p->getDescriptor().getLabel(),
                                                                      p->getDescriptor().getLongLabel() );
        
        QStringList groups = OfxEffectInstance::makePluginGrouping(p->getIdentifier(),
                                                                   p->getVersionMajor(), p->getVersionMinor(),
                                                                   pluginLabel, grouping);

        assert( p->getBinary() );
        QString iconFilename = QString( bundlePath.c_str() ) + "/Contents/Resources/";
        std::string pngIcon;
        try {
            // kOfxPropIcon is normally only defined for parameter desctriptors
            // (see <http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParameterProperties>)
            // but let's assume it may also be defained on the plugin descriptor.
            pngIcon = p->getDescriptor().getProps().getStringProperty(kOfxPropIcon, 1); // dimension 1 is PNG icon
        } catch (OFX::Host::Property::Exception) {
        }
        if (pngIcon.empty()) {
            // no icon defined by kOfxPropIcon, use the default value
            pngIcon = openfxId + ".png";
        }
        iconFilename.append( pngIcon.c_str() );
        QString groupIconFilename;
        if (groups.size() > 0) {
            groupIconFilename = QString( p->getBinary()->getBundlePath().c_str() ) + "/Contents/Resources/";
            // the plugin grouping has no descriptor, just try the default filename.
            groupIconFilename.append(groups[0]);
            groupIconFilename.append(".png");
        } else {
            //Use default Misc group when the plug-in doesn't belong to a group
            groups.push_back(PLUGIN_GROUP_DEFAULT);
        }

        
        const std::set<std::string> & contexts = p->getContexts();
        std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
        std::set<std::string>::const_iterator foundWriter = contexts.find(kOfxImageEffectContextWriter);
        
        bool userCreatable = openfxId != PLUGINID_OFX_ROTO;
        
        Natron::Plugin* natronPlugin = appPTR->registerPlugin( groups,
                                                              openfxId.c_str(),
                                                              pluginLabel.c_str(),
                                                              iconFilename,
                                                              groupIconFilename,
                                                              foundReader != contexts.end(),
                                                              foundWriter != contexts.end(),
                                                              new Natron::LibraryBinary(Natron::LibraryBinary::eLibraryTypeBuiltin),
                                                              p->getDescriptor().getRenderThreadSafety() == kOfxImageEffectRenderUnsafe,
                                                              p->getVersionMajor(), p->getVersionMinor(),userCreatable );
        
        natronPlugin->setOfxPlugin(p);
        
        ///if this plugin's descriptor has the kTuttleOfxImageEffectPropSupportedExtensions property,
        ///use it to fill the readersMap and writersMap
        int formatsCount = p->getDescriptor().getProps().getDimension(kTuttleOfxImageEffectPropSupportedExtensions);
        std::vector<std::string> formats(formatsCount);
        for (int k = 0; k < formatsCount; ++k) {
            formats[k] = p->getDescriptor().getProps().getStringProperty(kTuttleOfxImageEffectPropSupportedExtensions,k);
            std::transform(formats[k].begin(), formats[k].end(), formats[k].begin(), ::tolower);
        }

        double evaluation = p->getDescriptor().getProps().getDoubleProperty(kTuttleOfxImageEffectPropEvaluation);
        
        


        if ( ( foundReader != contexts.end() ) && (formatsCount > 0) && readersMap ) {
            ///we're safe to assume that this plugin is a reader
            for (U32 k = 0; k < formats.size(); ++k) {
                std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it;
                it = readersMap->find(formats[k]);

                if ( it != readersMap->end() ) {
                    it->second.push_back(std::make_pair(openfxId, evaluation));
                } else {
                    std::vector<std::pair<std::string,double> > newVec(1);
                    newVec[0] = std::make_pair(openfxId,evaluation);
                    readersMap->insert( std::make_pair(formats[k], newVec) );
                }
            }
        } else if ( ( foundWriter != contexts.end() ) && (formatsCount > 0) && writersMap ) {
            ///we're safe to assume that this plugin is a writer.
            for (U32 k = 0; k < formats.size(); ++k) {
                std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it;
                it = writersMap->find(formats[k]);

                if ( it != writersMap->end() ) {
                    it->second.push_back(std::make_pair(openfxId, evaluation));
                } else {
                    std::vector<std::pair<std::string,double> > newVec(1);
                    newVec[0] = std::make_pair(openfxId,evaluation);
                    writersMap->insert( std::make_pair(formats[k], newVec) );
                }
            }
        }
    }
} // loadOFXPlugins

void
Natron::OfxHost::writeOFXCache()
{
    /// and write a new cache, long version with everything in there
    QString ofxcachename = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationCache);

    QDir().mkpath(ofxcachename);
    ofxcachename +=  QDir::separator();
    ofxcachename += "OFXCache.xml";
    std::ofstream of( ofxcachename.toStdString().c_str() );
    assert( of.is_open() );
    assert( OFX::Host::PluginCache::getPluginCache() );
    OFX::Host::PluginCache::getPluginCache()->writePluginCache(of);
    of.close();
}

void
Natron::OfxHost::clearPluginsLoadedCache()
{
    QString ofxcachename = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationCache);

    QDir().mkpath(ofxcachename);
    ofxcachename +=  QDir::separator();
    ofxcachename += "OFXCache.xml";

    if ( QFile::exists(ofxcachename) ) {
        QFile::remove(ofxcachename);
    }
}

void
Natron::OfxHost::loadingStatus(const std::string & pluginId)
{
    if (appPTR) {
        appPTR->setLoadingStatus( "OpenFX: " + QString( pluginId.c_str() ) );
    }
}

bool
Natron::OfxHost::pluginSupported(OFX::Host::ImageEffect::ImageEffectPlugin */*plugin*/,
                                 std::string & /*reason*/) const
{
    ///Update: we support all bit depths and all components.


    // check that the plugin supports kOfxBitDepthFloat
//    if (plugin->getDescriptor().getParamSetProps().findStringPropValueIndex(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthFloat) == -1) {
//        reason = "32-bits floating-point bit depth not supported by plugin";
//        return false;
//    }

    return true;
}

const void*
Natron::OfxHost::fetchSuite(const char *suiteName,
                            int suiteVersion)
{
    if ( (strcmp(suiteName, kOfxParametricParameterSuite) == 0) && (suiteVersion == 1) ) {
        return OFX::Host::ParametricParam::GetSuite(suiteVersion);
    } else {
        return OFX::Host::ImageEffect::Host::fetchSuite(suiteName, suiteVersion);
    }
}

OFX::Host::Memory::Instance*
Natron::OfxHost::newMemoryInstance(size_t nBytes)
{
    OfxMemory* ret = new OfxMemory(NULL);
    bool allocated = ret->alloc(nBytes);
    
    if ((nBytes != 0 && !ret->getPtr()) || !allocated) {
        Natron::errorDialog(QObject::tr("Out of memory").toStdString(),
                            QObject::tr("Failed to allocate memory (").toStdString() + printAsRAM(nBytes).toStdString() + ").");
    }
    
    return ret;
}

/////////////////
/////////////////////////////////////////////////// MULTI_THREAD SUITE ///////////////////////////////////////////////////
/////////////////


#ifdef OFX_SUPPORTS_MULTITHREAD


///Stored as int, because we need -1; list because we need it recursive for the multiThread func
static QThreadStorage<std::list<int> > gThreadIndex;


void
Natron::OfxHost::setThreadAsActionCaller(bool actionCaller)
{
    if (actionCaller) {
        gThreadIndex.localData().push_back(-1);
    } else {
        std::list<int>& local = gThreadIndex.localData();
        assert(!local.empty());
        local.pop_back();
    }
}

namespace {
    
///Using QtConcurrent doesn't work with The Foundry Furnace plug-ins because they expect fresh threads
///to be created. As QtConcurrent's thread-pool recycles thread, it seems to make Furnace crash.
///We think this is because Furnace must keep an internal thread-local state that becomes then dirty
///if we re-use the same thread.

static OfxStatus
threadFunctionWrapper(OfxThreadFunctionV1 func,
                      unsigned int threadIndex,
                      unsigned int threadMax,
                      const std::map<boost::shared_ptr<Natron::Node>,ParallelRenderArgs >& tlsCopy,
                      void *customArg)
{
    assert(threadIndex < threadMax);
    std::list<int>& localData = gThreadIndex.localData();
    localData.push_back((int)threadIndex);
    
    boost::shared_ptr<ParallelRenderArgsSetter> tlsRaii;
    //Set the TLS if not NULL
    if (!tlsCopy.empty()) {
        tlsRaii.reset(new ParallelRenderArgsSetter(tlsCopy));
    }

    OfxStatus ret = kOfxStatOK;
    try {
        func(threadIndex, threadMax, customArg);
    } catch (const std::bad_alloc & ba) {
        ret =  kOfxStatErrMemory;
    } catch (...) {
        ret =  kOfxStatFailed;
    }

    ///reset back the index otherwise it could mess up the indexes if the same thread is re-used
    localData.pop_back();

    return ret;
}

    

    
class OfxThread
    : public QThread
{
public:
    OfxThread(OfxThreadFunctionV1 func,
              unsigned int threadIndex,
              unsigned int threadMax,
              const std::map<boost::shared_ptr<Natron::Node>,ParallelRenderArgs >& tlsCopy,
              void *customArg,
              OfxStatus *stat)
    : _func(func)
    , _threadIndex(threadIndex)
    , _threadMax(threadMax)
    , _tlsCopy(tlsCopy)
    , _customArg(customArg)
    , _stat(stat)
    {
        setObjectName("Multi-thread suite");
    }

    void run() OVERRIDE
    {
        assert(_threadIndex < _threadMax);
        std::list<int>& localData = gThreadIndex.localData();
        localData.push_back((int)_threadIndex);
        
        //Copy the TLS of the caller thread to the newly spawned thread
        boost::shared_ptr<ParallelRenderArgsSetter> tlsRaii;
        if (!_tlsCopy.empty()) {
            tlsRaii.reset(new ParallelRenderArgsSetter(_tlsCopy));
        }
        
        assert(*_stat == kOfxStatFailed);
        try {
            _func(_threadIndex, _threadMax, _customArg);
            *_stat = kOfxStatOK;
        } catch (const std::bad_alloc & ba) {
            *_stat = kOfxStatErrMemory;
        } catch (...) {
        }

        ///reset back the index otherwise it could mess up the indexes if the same thread is re-used
        localData.pop_back();
    }

private:
    OfxThreadFunctionV1 *_func;
    unsigned int _threadIndex;
    unsigned int _threadMax;
    std::map<boost::shared_ptr<Natron::Node>,ParallelRenderArgs > _tlsCopy;
    void *_customArg;
    OfxStatus *_stat;
};

}


// Function to spawn SMP threads
//  This function will spawn nThreads separate threads of computation (typically one per CPU) to allow something to perform symmetric multi processing. Each thread will call 'func' passing in the index of the thread and the number of threads actually launched.
// multiThread will not return until all the spawned threads have returned. It is up to the host how it waits for all the threads to return (busy wait, blocking, whatever).
// nThreads can be more than the value returned by multiThreadNumCPUs, however the threads will be limitted to the number of CPUs returned by multiThreadNumCPUs.
// This function cannot be called recursively.
// Note that the thread indexes are from 0 to nThreads-1.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThread

OfxStatus
Natron::OfxHost::multiThread(OfxThreadFunctionV1 func,
                             unsigned int nThreads,
                             void *customArg)
{
    if (!func) {
        return kOfxStatFailed;
    }

    unsigned int maxConcurrentThread;
    OfxStatus st = multiThreadNumCPUS(&maxConcurrentThread);
    if (st != kOfxStatOK) {
        return st;
    }

    // from the documentation:
    // "nThreads can be more than the value returned by multiThreadNumCPUs, however
    // the threads will be limitted to the number of CPUs returned by multiThreadNumCPUs."

    if ( (nThreads == 1) || (maxConcurrentThread <= 1) || (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) ) {
        try {
            for (unsigned int i = 0; i < nThreads; ++i) {
                func(i, nThreads, customArg);
            }

            return kOfxStatOK;
        } catch (...) {
            return kOfxStatFailed;
        }
    }
    
    //Retrieve a handle to the thread calling this action if possible so we can copy the TLS
    std::map<boost::shared_ptr<Natron::Node>,ParallelRenderArgs > tlsCopy;
    QVariant imageEffectPointerProperty = QThread::currentThread()->property(kNatronTLSEffectPointerProperty);
    if (!imageEffectPointerProperty.isNull()) {
        QObject* pointerqobject = imageEffectPointerProperty.value<QObject*>();
        if (pointerqobject) {
            Natron::EffectInstance* instance = dynamic_cast<Natron::EffectInstance*>(pointerqobject);
            if (instance) {
                instance->getApp()->getProject()->getParallelRenderArgs(tlsCopy);
            }
        }
    }

    bool useThreadPool = appPTR->getUseThreadPool();
    
    if (useThreadPool) {
        
        std::vector<unsigned int> threadIndexes(nThreads);
        for (unsigned int i = 0; i < nThreads; ++i) {
            threadIndexes[i] = i;
        }
        
        /// DON'T set the maximum thread count, this is a global application setting, and see the documentation excerpt above
        //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);
        QFuture<OfxStatus> future = QtConcurrent::mapped( threadIndexes, boost::bind(::threadFunctionWrapper,func, _1, nThreads, tlsCopy, customArg) );
        future.waitForFinished();
        ///DON'T reset back to the original value the maximum thread count
        //QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
        
        for (QFuture<OfxStatus>::const_iterator it = future.begin(); it != future.end(); ++it) {
            OfxStatus stat = *it;
            if (stat != kOfxStatOK) {
                return stat;
            }
        }

    } else {
        QVector<OfxStatus> status(nThreads); // vector for the return status of each thread
        status.fill(kOfxStatFailed); // by default, a thread fails
        {
            // at most maxConcurrentThread should be running at the same time
            QVector<OfxThread*> threads(nThreads);
            for (unsigned int i = 0; i < nThreads; ++i) {
                threads[i] = new OfxThread(func, i, nThreads, tlsCopy, customArg, &status[i]);
            }
            unsigned int i = 0; // index of next thread to launch
            unsigned int running = 0; // number of running threads
            unsigned int j = 0; // index of first running thread. all threads before this one are finished running
            while (j < nThreads) {
                // have no more than maxConcurrentThread threads launched at the same time
                int threadsStarted = 0;
                while (i < nThreads && running < maxConcurrentThread) {
                    threads[i]->start();
                    ++i;
                    ++running;
                    ++threadsStarted;
                }
                
                ///We just started threadsStarted threads
                appPTR->fetchAndAddNRunningThreads(threadsStarted);
                
                // now we've got at most maxConcurrentThread running. wait for each thread and launch a new one
                threads[j]->wait();
                assert( !threads[j]->isRunning() );
                assert( threads[j]->isFinished() );
                delete threads[j];
                ++j;
                --running;
                
                ///We just stopped 1 thread
                appPTR->fetchAndAddNRunningThreads(-1);
            }
            assert(running == 0);
        }
        // check the return status of each thread, return the first error found
        for (QVector<OfxStatus>::const_iterator it = status.begin(); it != status.end(); ++it) {
            OfxStatus stat = *it;
            if (stat != kOfxStatOK) {
                return stat;
            }
        }
    } // useThreadPool

    return kOfxStatOK;
} // multiThread

// Function which indicates the number of CPUs available for SMP processing
//  This value may be less than the actual number of CPUs on a machine, as the host may reserve other CPUs for itself.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadNumCPUs
OfxStatus
Natron::OfxHost::multiThreadNumCPUS(unsigned int *nCPUs) const
{
    if (!nCPUs) {
        return kOfxStatFailed;
    }
    
    int nThreadsToRender,nThreadsPerEffect;
    appPTR->getNThreadsSettings(&nThreadsToRender, &nThreadsPerEffect);
    
    if (nThreadsToRender == -1) {
        *nCPUs = 1;
    } else {
        // activeThreadCount may be negative (for example if releaseThread() is called)
        int activeThreadsCount = QThreadPool::globalInstance()->activeThreadCount();
        
        // Add the number of threads already running by the multiThreadSuite + parallel renders
        activeThreadsCount += appPTR->getNRunningThreads();
        
        // Clamp to 0
        activeThreadsCount = std::max( 0, activeThreadsCount);
        
        assert(activeThreadsCount >= 0);
        
        // better than QThread::idealThreadCount();, because it can be set by a global preference:
        int maxThreadsCount = QThreadPool::globalInstance()->maxThreadCount();
        assert(maxThreadsCount >= 0);
        
        if (nThreadsPerEffect == 0) {
            ///Simple heuristic: limit 1 effect to start at most 8 threads because otherwise it might spend too much
            ///time scheduling than just processing
            int hwConcurrency = appPTR->getHardwareIdealThreadCount();
            
            if (hwConcurrency <= 0) {
                nThreadsPerEffect = 1;
            } else if (hwConcurrency <= 4) {
                nThreadsPerEffect = hwConcurrency;
            } else {
                nThreadsPerEffect = 4;
            }
        }
        ///+1 because the current thread is going to wait during the multiThread call so we're better off
        ///not counting it.
        *nCPUs = std::max(1,std::min(maxThreadsCount - activeThreadsCount + 1, nThreadsPerEffect));
    }

    return kOfxStatOK;
}

// Function which indicates the index of the current thread
//  This function returns the thread index, which is the same as the threadIndex argument passed to the OfxThreadFunctionV1.
// If there are no threads currently spawned, then this function will set threadIndex to 0
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadIndex
// Note that the thread indexes are from 0 to nThreads-1, so a return value of 0 does not mean that it's not a spawned thread
// (use multiThreadIsSpawnedThread() to check if it's a spawned thread)
OfxStatus
Natron::OfxHost::multiThreadIndex(unsigned int *threadIndex) const
{
    if (!threadIndex) {
        return kOfxStatFailed;
    }

    if (!gThreadIndex.hasLocalData()) {
        *threadIndex = 0;
    } else {
        std::list<int>& localData = gThreadIndex.localData();
        if (!localData.empty() && localData.back() != -1) {
            *threadIndex = localData.back();
        } else {
            *threadIndex = 0;
        }
    }


    return kOfxStatOK;
}

// Function to enquire if the calling thread was spawned by multiThread
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadIsSpawnedThread
int
Natron::OfxHost::multiThreadIsSpawnedThread() const
{
    if (!gThreadIndex.hasLocalData()) {
        return 0;
    } else {
        std::list<int>& localData = gThreadIndex.localData();
        return !localData.empty() && localData.back() != -1;
    }
}

// Create a mutex
//  Creates a new mutex with lockCount locks on the mutex initially set.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexCreate
OfxStatus
Natron::OfxHost::mutexCreate(OfxMutexHandle *mutex,
                             int lockCount)
{
    if (!mutex) {
        return kOfxStatFailed;
    }

    // suite functions should not throw
    try {
        QMutex* m = new QMutex(QMutex::Recursive);
        for (int i = 0; i < lockCount; ++i) {
            m->lock();
        }
        *mutex = (OfxMutexHandle)(m);
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
        {
            QMutexLocker l(_pluginsMutexesLock);
            _pluginsMutexes.push_back(m);
        }
#endif
        return kOfxStatOK;
    } catch (std::bad_alloc) {
        qDebug() << "mutexCreate(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexCreate(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexCreate(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Destroy a mutex
//  Destroys a mutex intially created by mutexCreate.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexDestroy
OfxStatus
Natron::OfxHost::mutexDestroy(const OfxMutexHandle mutex)
{
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
        const QMutex* mutexqt = reinterpret_cast<const QMutex*>(mutex);
        {
            QMutexLocker l(_pluginsMutexesLock);
            std::list<QMutex*>::iterator found = std::find(_pluginsMutexes.begin(),_pluginsMutexes.end(),mutexqt);
            if ( found != _pluginsMutexes.end() ) {
                delete *found;
                _pluginsMutexes.erase(found);
            }
        }
#else
        delete reinterpret_cast<const QMutex*>(mutex);
#endif

        return kOfxStatOK;
    } catch (std::bad_alloc) {
        qDebug() << "mutexDestroy(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexDestroy(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexDestroy(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Blocking lock on the mutex
//  This trys to lock a mutex and blocks the thread it is in until the lock suceeds.
// A sucessful lock causes the mutex's lock count to be increased by one and to block any other calls to lock the mutex until it is unlocked.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexLock
OfxStatus
Natron::OfxHost::mutexLock(const OfxMutexHandle mutex)
{
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        reinterpret_cast<QMutex*>(mutex)->lock();

        return kOfxStatOK;
    } catch (std::bad_alloc) {
        qDebug() << "mutexLock(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexLock(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexLock(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Unlock the mutex
//  This unlocks a mutex. Unlocking a mutex decreases its lock count by one.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexUnLock
OfxStatus
Natron::OfxHost::mutexUnLock(const OfxMutexHandle mutex)
{
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        reinterpret_cast<QMutex*>(mutex)->unlock();

        return kOfxStatOK;
    } catch (std::bad_alloc) {
        qDebug() << "mutexUnLock(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexUnLock(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexUnLock(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Non blocking attempt to lock the mutex
//  This attempts to lock a mutex, if it cannot, it returns and says so, rather than blocking.
// A sucessful lock causes the mutex's lock count to be increased by one, if the lock did not suceed, the call returns immediately and the lock count remains unchanged.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexTryLock
OfxStatus
Natron::OfxHost::mutexTryLock(const OfxMutexHandle mutex)
{
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        if ( reinterpret_cast<QMutex*>(mutex)->tryLock() ) {
            return kOfxStatOK;
        } else {
            return kOfxStatFailed;
        }
    } catch (std::bad_alloc) {
        qDebug() << "mutexTryLock(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexTryLock(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexTryLock(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

#endif // ifdef OFX_SUPPORTS_MULTITHREAD

