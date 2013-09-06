//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "OfxParamInstance.h"

#include <iostream>
#include <QColor>
#include <QHBoxLayout>
//ofx extension
#include <nuke/fnPublicOfxExtensions.h>

#include "Gui/KnobGui.h"
#include "Engine/OfxNode.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"

using namespace std;
using namespace Powiter;

OfxPushButtonInstance::OfxPushButtonInstance(OfxNode* node,
                                             const std::string& name,
                                             OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::PushbuttonInstance(descriptor, node->effectInstance())
, _node(node)
, _descriptor(descriptor)
{
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<Button_Knob*>(KnobFactory::createKnob("Button", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    _knob->connectButtonToSlot(this, SLOT(emitInstanceChanged()));
    QObject::connect(this, SIGNAL(buttonPressed(QString)), _node, SLOT(onInstanceChangedAction(QString)));
}

void OfxPushButtonInstance::emitInstanceChanged(){
    emit buttonPressed(getName().c_str());
}


// callback which should set enabled state as appropriate
void OfxPushButtonInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxPushButtonInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
KnobGui* OfxPushButtonInstance::getKnob() const{
    return _knob;
}

OfxIntegerInstance::OfxIntegerInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::IntegerInstance(descriptor, node->effectInstance())
, _node(node)
, _descriptor(descriptor)
, _paramName(name)
{
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<Int_Knob*>(KnobFactory::createKnob("Int", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    QObject::connect(_knob, SIGNAL(valueChanged(int)), this, SLOT(onInstanceChanged()));
    _knob->setPointer(&_value);
    int min = getProperties().getIntProperty(kOfxParamPropDisplayMin);
    int max = getProperties().getIntProperty(kOfxParamPropDisplayMax);
    int def = getProperties().getIntProperty(kOfxParamPropDefault);
    _knob->setMinimum(min);
    _knob->setMaximum(max);
    _knob->setValue(def);
}
OfxStatus OfxIntegerInstance::get(int& v) {
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::get(OfxTime /*time*/, int& v) {
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(int v){
    _value = v;
    _knob->setValue(v);
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(OfxTime /*time*/, int v){
    _value = v;
    _knob->setValue(v);
    return kOfxStatOK;
}
void OfxIntegerInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}
// callback which should set enabled state as appropriate
void OfxIntegerInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxIntegerInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
KnobGui* OfxIntegerInstance::getKnob() const{
    return _knob;
}

OfxDoubleInstance::OfxDoubleInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::DoubleInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<Double_Knob*>(KnobFactory::createKnob("Double", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    QObject::connect(_knob, SIGNAL(valueChanged(double)), this, SLOT(onInstanceChanged()));
    _knob->setPointer(&_value);
    double min = getProperties().getDoubleProperty(kOfxParamPropDisplayMin);
    double max = getProperties().getDoubleProperty(kOfxParamPropDisplayMax);
    double incr = getProperties().getDoubleProperty(kOfxParamPropIncrement);
    double def = getProperties().getDoubleProperty(kOfxParamPropDefault);
    _knob->setMinimum(min);
    _knob->setMaximum(max);
    _knob->setIncrement(incr);
    _knob->setValue(def);
}
OfxStatus OfxDoubleInstance::get(double& v){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::get(OfxTime /*time*/, double& v){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(double v) {
    _value = v;
    _knob->setValue(v);
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(OfxTime /*time*/, double v){
    _value = v;
    _knob->setValue(v);
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::derive(OfxTime /*time*/, double& /*v*/){
    return kOfxStatErrMissingHostFeature;
}
OfxStatus OfxDoubleInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, double& /*v*/){
    return kOfxStatErrMissingHostFeature;
}
void OfxDoubleInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}
// callback which should set enabled state as appropriate
void OfxDoubleInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDoubleInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
KnobGui* OfxDoubleInstance::getKnob() const{
    return _knob;
}

OfxBooleanInstance::OfxBooleanInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::BooleanInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<Bool_Knob*>(KnobFactory::createKnob("Bool", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    QObject::connect(_knob, SIGNAL(triggered(bool)), this, SLOT(onInstanceChanged()));
    _knob->setPointer(&_value);
    int def = getProperties().getIntProperty(kOfxParamPropDefault);
    _knob->setChecked(def);
    
}
OfxStatus OfxBooleanInstance::get(bool& b){
    b = _value;
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::get(OfxTime /*time*/, bool& b) {
    b = _value;
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::set(bool b){
    _value = b;
    _knob->setChecked(b);
    return kOfxStatOK;
}

OfxStatus OfxBooleanInstance::set(OfxTime /*time*/, bool b){
    _value = b;
    _knob->setChecked(b);
    return kOfxStatOK;
}
void OfxBooleanInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}
// callback which should set enabled state as appropriate
void OfxBooleanInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxBooleanInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
KnobGui* OfxBooleanInstance::getKnob() const{
    return _knob;
}


OfxChoiceInstance::OfxChoiceInstance(OfxNode *node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::ChoiceInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name) {
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<ComboBox_Knob*>(KnobFactory::createKnob("ComboBox", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    
    QObject::connect(_knob, SIGNAL(entryChanged(int)), this, SLOT(onInstanceChanged()));
    OFX::Host::Property::Set& pSet = getProperties();
    for (int i = 0 ; i < pSet.getDimension(kOfxParamPropChoiceOption) ; ++i) {
        _entries.push_back(pSet.getStringProperty(kOfxParamPropChoiceOption,i));
    }
    _knob->setPointer(&_currentEntry);
    _knob->populate(_entries);
    int def = pSet.getIntProperty(kOfxParamPropDefault);
    
    set(def);
}
OfxStatus OfxChoiceInstance::get(int& v){
    for (unsigned int i = 0; i < _entries.size(); ++i) {
        if (_entries[i] == _currentEntry) {
            v = i;
            return kOfxStatOK;
        }
    }
    return kOfxStatErrBadIndex;
}
OfxStatus OfxChoiceInstance::get(OfxTime /*time*/, int& v){
    for (unsigned int i = 0; i < _entries.size(); ++i) {
        if (_entries[i] == _currentEntry) {
            v = i;
            return kOfxStatOK;
        }
    }
    return kOfxStatErrBadIndex;
}
OfxStatus OfxChoiceInstance::set(int v){
    if(v < (int)_entries.size()){
        _knob->setCurrentItem(v);
        return kOfxStatOK;
    }else{
        return kOfxStatErrBadIndex;
    }
}
OfxStatus OfxChoiceInstance::set(OfxTime /*time*/, int v){
    if(v < (int)_entries.size()){
        _knob->setCurrentItem(v);
        return kOfxStatOK;
    }else{
        return kOfxStatErrBadIndex;
    }
}
void OfxChoiceInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}


// callback which should set enabled state as appropriate
void OfxChoiceInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxChoiceInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
KnobGui* OfxChoiceInstance::getKnob() const{
    return _knob;
}



OfxRGBAInstance::OfxRGBAInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::RGBAInstance(descriptor,node->effectInstance()),
_node(node),
_descriptor(descriptor),
_r(0),_g(0),_b(0),_a(1.0),
_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<RGBA_Knob*>(KnobFactory::createKnob("RGBA", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    _knob->setPointers(&_r, &_g,&_b,&_a);
    
    QObject::connect(_knob, SIGNAL(colorChanged(QColor)), this, SLOT(onInstanceChanged()));
    
    
    double defR = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    double defG = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    double defB = getProperties().getDoubleProperty(kOfxParamPropDefault,2);
    double defA = getProperties().getDoubleProperty(kOfxParamPropDefault,3);
    
    _knob->setRGBA(defR, defG, defB, defA);
}
OfxStatus OfxRGBAInstance::get(double& r, double& g, double& b, double& a) {
    r = _r;
    g = _g;
    b = _b;
    a = _a;
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::get(OfxTime /*time*/, double&r ,double& g, double& b, double& a) {
    r = _r;
    g = _g;
    b = _b;
    a = _a;
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::set(double r,double g , double b ,double a){
    _r = r;
    _g = g;
    _b = b;
    _a = a;
    _knob->setRGBA(r, g, b, a);
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::set(OfxTime /*time*/, double r ,double g,double b,double a){
    _r = r;
    _g = g;
    _b = b;
    _a = a;
    _knob->setRGBA(r, g, b, a);
    return kOfxStatOK;
}

void OfxRGBAInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}

// callback which should set enabled state as appropriate
void OfxRGBAInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxRGBAInstance::setSecret(){
    _knob->setVisible(!getSecret());
}


KnobGui* OfxRGBAInstance::getKnob() const{
    return _knob;
}
OfxRGBInstance::OfxRGBInstance(OfxNode *node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::RGBInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<RGBA_Knob*>(KnobFactory::createKnob("RGBA", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    _knob->disablePermantlyAlpha();
    _knob->setPointers(&_r, &_g,&_b,NULL);
    
    QObject::connect(_knob, SIGNAL(colorChanged(QColor)), this, SLOT(onInstanceChanged()));
    
    
    double defR = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    double defG = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    double defB = getProperties().getDoubleProperty(kOfxParamPropDefault,2);
    
    _knob->setRGBA(defR, defG, defB, 1.f);
    
}
OfxStatus OfxRGBInstance::get(double& r, double& g, double& b) {
    r = _r;
    g = _g;
    b = _b;
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::get(OfxTime /*time*/, double& r, double& g, double& b) {
    r = _r;
    g = _g;
    b = _b;
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::set(double r,double g,double b){
	_r = r;
	_g = g;
	_b = b;
    _knob->setRGBA(r, g, b, 1.);
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::set(OfxTime /*time*/, double r,double g,double b){
	_r = r;
	_g = g;
	_b = b;
    _knob->setRGBA(r, g, b, 1.);
    return kOfxStatOK;
}

void OfxRGBInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}
// callback which should set enabled state as appropriate
void OfxRGBInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxRGBInstance::setSecret(){
    _knob->setVisible(!getSecret());
}

KnobGui* OfxRGBInstance::getKnob() const{
    return _knob;
}

OfxDouble2DInstance::OfxDouble2DInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::Double2DInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<Double2D_Knob*>(KnobFactory::createKnob("Double2D", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    _knob->setPointers(&_x1, &_x2);
    
    QObject::connect(_knob, SIGNAL(value1Changed(double)), this, SLOT(onInstanceChanged()));
    QObject::connect(_knob, SIGNAL(value2Changed(double)), this, SLOT(onInstanceChanged()));
    
    double min1 = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,0);
    double max1 = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,0);
    double incr1 = getProperties().getDoubleProperty(kOfxParamPropIncrement,0);
    double def1 = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    
    double min2 = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,1);
    double max2 = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,1);
    double incr2 = getProperties().getDoubleProperty(kOfxParamPropIncrement,1);
    double def2 = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    
    _knob->setMinimum1(min1);
    _knob->setMaximum1(max1);
    _knob->setIncrement1(incr1);
    _knob->setValue1(def1);
    
    _knob->setMinimum2(min2);
    _knob->setMaximum2(max2);
    _knob->setIncrement2(incr2);
    _knob->setValue2(def2);
    
}
OfxStatus OfxDouble2DInstance::get(double& x1, double& x2) {
    x1 = _x1;
    x2 = _x2;
    return kOfxStatOK;
}
OfxStatus OfxDouble2DInstance::get(OfxTime /*time*/, double& x1, double& x2) {
    x1 = _x1;
    x2 = _x2;
    return kOfxStatOK;
}
OfxStatus OfxDouble2DInstance::set(double x1,double x2){
	_x1 = x1;
	_x2 = x2;
    _knob->setValue1(_x1);
    _knob->setValue2(_x2);
	return kOfxStatOK;
}
OfxStatus OfxDouble2DInstance::set(OfxTime /*time*/,double x1,double x2){
	_x1 = x1;
	_x2 = x2;
    _knob->setValue1(_x1);
    _knob->setValue2(_x2);
	return kOfxStatOK;
}
void OfxDouble2DInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}
// callback which should set enabled state as appropriate
void OfxDouble2DInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDouble2DInstance::setSecret(){
    _knob->setVisible(!getSecret());
}

KnobGui* OfxDouble2DInstance::getKnob() const{
    return _knob;
}

OfxInteger2DInstance::OfxInteger2DInstance(OfxNode *node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::Integer2DInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    _knob = dynamic_cast<Int2D_Knob*>(KnobFactory::createKnob("Int2D", cb, name, KnobGui::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    _knob->setPointers(&_x1, &_x2);
    
    QObject::connect(_knob, SIGNAL(value1Changed(int)), this, SLOT(onInstanceChanged()));
    QObject::connect(_knob, SIGNAL(value2Changed(int)), this, SLOT(onInstanceChanged()));
    
    int min1 = getProperties().getIntProperty(kOfxParamPropDisplayMin,0);
    int max1 = getProperties().getIntProperty(kOfxParamPropDisplayMax,0);
    int def1 = getProperties().getIntProperty(kOfxParamPropDefault,0);
    
    int min2 = getProperties().getIntProperty(kOfxParamPropDisplayMin,1);
    int max2 = getProperties().getIntProperty(kOfxParamPropDisplayMax,1);
    int def2 = getProperties().getIntProperty(kOfxParamPropDefault,1);
    
    _knob->setMinimum1(min1);
    _knob->setMaximum1(max1);
    _knob->setValue1(def1);
    
    _knob->setMinimum2(min2);
    _knob->setMaximum2(max2);
    _knob->setValue2(def2);
}
OfxStatus OfxInteger2DInstance::get(int& x1, int& x2) {
    x1 = _x1;
    x2 = _x2;
    return kOfxStatOK;
}
OfxStatus OfxInteger2DInstance::get(OfxTime /*time*/, int& x1, int& x2) {
    x1 = _x1;
    x2 = _x2;
    return kOfxStatOK;
}
OfxStatus OfxInteger2DInstance::set(int x1,int x2){
	_x1 = x1;
	_x2 = x2;
    _knob->setValue1(x1);
    _knob->setValue2(x2);
	return kOfxStatOK;
}
OfxStatus OfxInteger2DInstance::set(OfxTime /*time*/, int x1, int x2) {
	_x1 = x1;
	_x2 = x2;
    _knob->setValue1(x1);
    _knob->setValue2(x2);
	return kOfxStatOK;
}
void OfxInteger2DInstance::onInstanceChanged(){
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
}

// callback which should set enabled state as appropriate
void OfxInteger2DInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxInteger2DInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
KnobGui* OfxInteger2DInstance::getKnob() const{
    return _knob;
}


/***********/
OfxGroupInstance::OfxGroupInstance(OfxNode *node,const std::string& name,OFX::Host::Param::Descriptor& descriptor):
OFX::Host::Param::GroupInstance(descriptor,node->effectInstance()),_node(node),_descriptor(descriptor),_paramName(name){
    KnobCallback* cb = _node->getKnobCallBack();
    int isTab = getProperties().getIntProperty(kFnOfxParamPropGroupIsTab);
    if(isTab){
        Tab_Knob* _tabKnob = _node->getTabKnob();
        if(!_tabKnob){
            _tabKnob = dynamic_cast<Tab_Knob*>(KnobFactory::createKnob("Tab", cb, name, KnobGui::NONE));
            _node->setTabKnob(_tabKnob);
        }
        _groupKnob = 0;
        _tabKnob->addTab(name);
    }else{
        _groupKnob = dynamic_cast<Group_Knob*>(KnobFactory::createKnob("Group", cb, name, KnobGui::NONE));
        int opened = getProperties().getIntProperty(kOfxParamPropGroupOpen);
        if (opened) {
            _groupKnob->setChecked(true);
        }else{
            _groupKnob->setChecked(false);
        }
    }
    
}
void OfxGroupInstance::addKnob(KnobGui *k) {
    if(_groupKnob){
        _groupKnob->addKnob(k);
    }else{
        _node->getTabKnob()->addKnob(_paramName, k);
    }
}

KnobGui* OfxGroupInstance::getKnob() const{
    if(_groupKnob){
        return _groupKnob;
    }else{
        return _node->getTabKnob();
    }
}

OfxStringInstance::OfxStringInstance(OfxNode *node,const std::string& name,OFX::Host::Param::Descriptor& descriptor):
OFX::Host::Param::StringInstance(descriptor,node->effectInstance()),_node(node),_descriptor(descriptor),_paramName(name),
_fileKnob(0),_outputFileKnob(0){
    KnobCallback* cb = _node->getKnobCallBack();
    std::string mode = getProperties().getStringProperty(kOfxParamPropStringMode);
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        KnobFactory::createKnob("Separator", cb, name, KnobGui::NONE);
    }
    if(mode == kOfxParamStringIsFilePath){
        if(_node->isInputNode()){
            _fileKnob = dynamic_cast<File_Knob*>(KnobFactory::createKnob("InputFile", cb, name, KnobGui::NONE));
            _fileKnob->setPointer(&_filesList);
            QObject::connect(_fileKnob, SIGNAL(filesSelected()), this, SLOT(onInstanceChanged()));
            if(layoutHint == 2){
                _fileKnob->turnOffNewLine();
            }
            _fileKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
        }else{
            _node->setAsOutputNode(); // IMPORTANT ! 
            _outputFileKnob = dynamic_cast<OutputFile_Knob*>(KnobFactory::createKnob("OutputFile", cb, name, KnobGui::NONE));
            _outputFileKnob->setPointer(&_outputFilePattern);
            QObject::connect(_outputFileKnob, SIGNAL(filesSelected()), this, SLOT(onInstanceChanged()));
            if(layoutHint == 2){
                _outputFileKnob->turnOffNewLine();
            }
            _outputFileKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
        }
    }else if(mode == kOfxParamStringIsSingleLine || mode == kOfxParamStringIsLabel){
        KnobGui::Knob_Flags flags = KnobGui::NONE;
        if(mode == kOfxParamStringIsLabel){
            flags = KnobGui::READ_ONLY;
        }
        _stringKnob = dynamic_cast<String_Knob*>(KnobFactory::createKnob("String", cb, name, flags));
        QObject::connect(_stringKnob, SIGNAL(stringChanged(QString)), this, SLOT(onInstanceChanged()));
        _stringKnob->setPointer(&_returnValue);
        if(layoutHint == 2){
            _stringKnob->turnOffNewLine();
        }
        _stringKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    }
    
}
OfxStatus OfxStringInstance::get(std::string &str) {
    if(_fileKnob){
        int currentFrame = clampToRange((int)_node->effectInstance()->timeLineGetTime());
        if(currentFrame != INT_MAX && currentFrame != INT_MIN){
            map<int,QString>::iterator it = _files.find(currentFrame);
            if(it!=_files.end()){
                str = it->second.toStdString();
            }else{
                str = "";
            }
        }else{
            str = "";
        }
    }else if(_outputFileKnob){
        str = filenameFromPattern(_node->effectInstance()->timeLineGetTime());
    }else{
        str = _returnValue;
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::get(OfxTime /*time*/, std::string& str) {
    if(_fileKnob){
        int currentFrame = clampToRange((int)_node->effectInstance()->timeLineGetTime());
        if(currentFrame != INT_MAX && currentFrame != INT_MIN){
            map<int,QString>::iterator it = _files.find(currentFrame);
            if(it!=_files.end()){
                str = it->second.toStdString();
            }else{
                str = "";
            }
        }else{
            str = "";
        }
    }else if(_outputFileKnob){
        str = filenameFromPattern(_node->effectInstance()->timeLineGetTime());
    }else{
        str = _returnValue;
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::set(const char* str) {
    _returnValue = str;
    if(_fileKnob){
        _fileKnob->setLineEditText(_returnValue);
    }
    if(_outputFileKnob){
        _outputFileKnob->setStr(str);
    }
    if(_stringKnob){
        _stringKnob->setString(str);
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::set(OfxTime /*time*/, const char* str) {
    _returnValue = str;
    if(_fileKnob){
        _fileKnob->setLineEditText(_returnValue);
    }
    if(_outputFileKnob){
         _outputFileKnob->setStr(str);
    }
    if(_stringKnob){
        _stringKnob->setString(str);
    }
    return kOfxStatOK;
}

KnobGui* OfxStringInstance::getKnob() const{
    
    if(_fileKnob){
        return _fileKnob;
    }
    if(_outputFileKnob){
        return _outputFileKnob;
    }
    if(_stringKnob){
        return _stringKnob;
    }
    return NULL;
}
// callback which should set enabled state as appropriate
void OfxStringInstance::setEnabled(){
    if(_fileKnob){
        _fileKnob->setEnabled(getEnabled());
    }
    if (_outputFileKnob) {
        _outputFileKnob->setEnabled(getEnabled());
    }
    if (_stringKnob) {
        _stringKnob->setEnabled(getEnabled());
    }
}

// callback which should set secret state as appropriate
void OfxStringInstance::setSecret(){
    if(_fileKnob){
        _fileKnob->setVisible(!getSecret());
    }
    if (_outputFileKnob) {
        _outputFileKnob->setVisible(getSecret());
    }
    if (_stringKnob) {
        _stringKnob->setVisible(getSecret());
    }
    
}
void OfxStringInstance::onInstanceChanged(){
    if(_fileKnob){
        getVideoSequenceFromFilesList();
        _returnValue = _fileKnob->getLineEditText();
         _node->computePreviewImage();

    }
    _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
    _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);

}
void OfxStringInstance::getVideoSequenceFromFilesList(){
    _files.clear();
    bool first_time=true;
    QString originalName;
    foreach(QString Qfilename,_filesList)
    {	if(Qfilename.at(0) == QChar('.')) continue;
        QString const_qfilename=Qfilename;
        if(first_time){
            Qfilename=Qfilename.remove(Qfilename.length()-4,4);
            int j=Qfilename.length()-1;
            QString frameIndex;
            while(j>0 && Qfilename.at(j).isDigit()){
                frameIndex.push_front(Qfilename.at(j));
                --j;
            }
            if(j>0){
				int number=0;
                if(_filesList.size() > 1){
                    number = frameIndex.toInt();
                }
				_files.insert(make_pair(number,const_qfilename));
                originalName=Qfilename.remove(j+1,frameIndex.length());
                
            }else{
                _files[0]=const_qfilename;
            }
            first_time=false;
        }else{
            if(Qfilename.contains(originalName) /*&& (extension)*/){
                Qfilename.remove(Qfilename.length()-4,4);
                int j=Qfilename.length()-1;
                QString frameIndex;
                while(j>0 && Qfilename.at(j).isDigit()){
                    frameIndex.push_front(Qfilename.at(j));
                    --j;
                }
                if(j>0){
                    int number = frameIndex.toInt();
                    _files[number]=const_qfilename;
                }else{
                    cout << " Read handle : WARNING !! several frames read but no frame count found in their name " << endl;
                }
            }
        }
    }
    _node->set_firstFrame(firstFrame());
    _node->set_lastFrame(lastFrame());
    
}

int OfxStringInstance::firstFrame(){
    std::map<int,QString>::iterator it=_files.begin();
    if(it == _files.end()) return INT_MIN;
    return it->first;
}
int OfxStringInstance::lastFrame(){
    std::map<int,QString>::iterator it=_files.end();
    if(it == _files.begin()) return INT_MAX;
    --it;
    return it->first;
}
int OfxStringInstance::clampToRange(int f){
    int first = firstFrame();
    int last = lastFrame();
    if(f < first) return first;
    if(f > last) return last;
    return f;
}
bool OfxStringInstance::isValid() const{
    if(_fileKnob){
        return !_filesList.isEmpty();
    }
    if(_outputFileKnob){
        return !_outputFilePattern.empty();
    }
    return true;
}
std::string OfxStringInstance::filenameFromPattern(int frameIndex) const{
    if(_outputFileKnob){
        if(isValid()){
            QString p(_outputFilePattern.c_str());
            return p.replace("#", QString::number(frameIndex)).toStdString();
        }
    }
    return "";
}
