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

#include "Writer.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtConcurrentRun>

#include "Global/AppManager.h"
#include "Engine/Row.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"
#include "Engine/PluginID.h"
#include "Engine/Settings.h"
#include "Writers/Write.h"
#include "Gui/Knob.h"

using namespace std;
using namespace Powiter;

Writer::Writer():
OutputNode(),
_requestedChannels(Mask_RGB), // temporary
_currentFrame(0),
_premult(false),
_buffer(Settings::getPowiterCurrentSettings()->_writersSettings._maximumBufferSize),
_writeHandle(0),
_writeOptions(0)
{
     
    _lock = new QMutex;
}

Writer::~Writer(){
    if(_writeOptions){
        //    _writeOptions->cleanUpKnobs();
        delete _writeOptions;
    }
    delete _lock;
}

std::string Writer::className() {
    return "Writer";
}

std::string Writer::description() {
    return "OutputNode";
}

bool Writer::_validate(bool forReal){
    /*Defaults writing range to readers range, but
     the user may change it through GUI.*/
    _frameRange.first = info().firstFrame();
    _frameRange.second = info().lastFrame();
    
    if (forReal) {
        
        
        if(_filename.size() > 0){
            
            Write* write = 0;
            PluginID* encoder = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
            if(!encoder){
                cout << "Writer: couldn't find an appropriate encoder for filetype: " << _fileType << endl;
            }else{
                WriteBuilder builder = (WriteBuilder)(encoder->first);
                write = builder(this);
                write->premultiplyByAlpha(_premult);
                /*check if the filename already contains the extension, otherwise appending it*/
                QString extension;
                QString filename(_filename.c_str());
                int i = filename.lastIndexOf(QChar('.'));
                if(i != -1){
                    extension.append(filename.toStdString().substr(i).c_str());
                    filename = filename.replace(i+1, extension.size(), _fileType.c_str());
                }else{
                    filename.append(extension);
                }
                
                i = filename.lastIndexOf(QChar('#'));
                QString n = QString::number(_currentFrame);
                if(i != -1){
                    filename = filename.replace(i,1,n);
                }else{
                    i = filename.lastIndexOf(QChar('.'));
                    filename = filename.insert(i, n);
                }
                
                write->setOptionalKnobsPtr(_writeOptions);
                write->setupFile(filename.toStdString());
                write->initializeColorSpace();
                _writeHandle = write;
            }
        }
    }
    return true;
}

void Writer::engine(int y,int offset,int range,ChannelSet channels,Row* out){
    _writeHandle->engine(y, offset, range, channels, out);
}

void Writer::createKnobDynamically(){
    Node::createKnobDynamically();
}
void Writer::initKnobs(KnobCallback *cb){
    std::string fileDesc("File");
    _fileKnob = static_cast<OutputFile_Knob*>(KnobFactory::createKnob("OutputFile", cb, fileDesc, Knob::NONE));
    assert(_fileKnob);
	_fileKnob->setPointer(&_filename);
    QObject::connect(_fileKnob, SIGNAL(filesSelected()), this, SLOT(onFilesSelected()));
    
    std::string renderDesc("Render");
    Button_Knob* renderButton = static_cast<Button_Knob*>(KnobFactory::createKnob("Button", cb, renderDesc, Knob::NONE));
    assert(renderButton);
    renderButton->connectButtonToSlot(dynamic_cast<QObject*>(this),SLOT(startRendering()));
    
    std::string premultString("Premultiply by alpha");
    Bool_Knob* premult = static_cast<Bool_Knob*>(KnobFactory::createKnob("Bool", cb, premultString, Knob::NONE));
    premult->setPointer(&_premult);
    
    std::string filetypeStr("File type");
    _filetypeCombo = static_cast<ComboBox_Knob*>(KnobFactory::createKnob("ComboBox", cb, filetypeStr, Knob::NONE));
    QObject::connect(_filetypeCombo, SIGNAL(entryChanged(int)), this, SLOT(fileTypeChanged(int)));
    const std::map<std::string,PluginID*>& _encoders = Settings::getPowiterCurrentSettings()->_writersSettings.getFileTypesMap();
    std::map<std::string,PluginID*>::const_iterator it = _encoders.begin();
    for(;it!=_encoders.end();++it) {
        _allFileTypes.push_back(it->first);
    }
    _filetypeCombo->setPointer(&_fileType);
    _filetypeCombo->populate(_allFileTypes);
    Node::initKnobs(cb);
}


void Writer::write(Write* write,QFutureWatcher<void>* watcher){
    
    _buffer.appendTask(write, watcher);
    if(!write) return;
    write->writeAndDelete();
    _buffer.removeTask(write);
}

void Writer::startWriting(){
    if(_buffer.size() < _buffer.getMaximumBufferSize()){
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, SIGNAL(finished()), this, SLOT(notifyWriterForCompletion()));
        QFuture<void> future = QtConcurrent::run(this,&Writer::write,_writeHandle,watcher);
        _writeHandle = 0;
        watcher->setFuture(future);
    }else{
        _writeQueue.push_back(_writeHandle);
        _writeHandle = 0;
    }
}

void Writer::notifyWriterForCompletion(){
    _buffer.emptyTrash();
    
    /*Several threads may fight here to try to start another task.
     We ensure that at least 1 thread get into the condition, as more
     are not needed. */
    QMutexLocker guard(_lock);
    if(_writeQueue.size() > 0 && _buffer.size() < _buffer.getMaximumBufferSize()){
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, SIGNAL(finished()), this, SLOT(notifyWriterForCompletion()));
        QFuture<void> future = QtConcurrent::run(this,&Writer::write,_writeQueue[0],watcher);
        _writeQueue.erase(_writeQueue.begin());
        watcher->setFuture(future);
    }
}

void Writer::Buffer::appendTask(Write* task,QFutureWatcher<void>* future){
    _tasks.push_back(make_pair(task, future));
}

void Writer::Buffer::removeTask(Write* task){
    for (U32 i = 0 ; i < _tasks.size(); ++i) {
        std::pair<Write*,QFutureWatcher<void>* >& t = _tasks[i];
        if(t.first == task){
            _trash.push_back(t.second);
            _tasks.erase(_tasks.begin()+i);
            break;
        }
    }
}
void Writer::Buffer::emptyTrash(){
    for (U32 i = 0; i < _trash.size(); ++i) {
        delete _trash[i];
    }
    _trash.clear();
}

Writer::Buffer::~Buffer(){
    for (U32 i = 0 ; i < _tasks.size(); ++i) {
        std::pair<Write*,QFutureWatcher<void>* >& t = _tasks[i];
        delete t.second;
    }
}

bool Writer::validInfosForRendering(){
    /*check if filetype is valid*/
    PluginID* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!isValid) return false;
    
    /*checking if channels are supported*/
    WriteBuilder builder = (WriteBuilder)isValid->first;
    Write* write = builder(NULL);
    try{
        write->supportsChannelsForWriting(_requestedChannels);
    }catch(const char* str){
        cout << "ERROR: " << str << endl;
    }
    delete write;
    
    /*check if frame range makes sense*/
    if(_frameRange.first > _frameRange.second) return false;
    
    /*check if write specific knobs have valid values*/
    if (_writeOptions) {
        if (!_writeOptions->allValid()) {
            return false;
        }
    }
    
    return true;
}

void Writer::startRendering(){
    if(validInfosForRendering()){
        appPTR->getModel()->setCurrentGraph(this,false);
        appPTR->getModel()->startVideoEngine();
    }
}

void Writer::fileTypeChanged(int fileTypeIndex){
    string fileType = _allFileTypes[fileTypeIndex];
    if(_writeOptions){
        _writeOptions->cleanUpKnobs();
        delete _writeOptions;
        _writeOptions = 0;
    }
    PluginID* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!isValid) return;
    
    QString file(_filename.c_str());
    int pos = file.lastIndexOf(QChar('.'));
    ++pos;
    file.replace(pos, file.size() - pos, _fileType.c_str());
    _fileKnob->setStr(file);
    
    /*checking if channels are supported*/
    WriteBuilder builder = (WriteBuilder)isValid->first;
    Write* write = builder(this);
    _writeOptions = write->initSpecificKnobs();
    if(_writeOptions)
        _writeOptions->initKnobs(getKnobCallBack(),fileType);
    delete write;
}
void Writer::onFilesSelected(){
    QString file(_filename.c_str());
    int pos = file.lastIndexOf(QChar('.'));
    string ext = _filename.substr(pos+1).c_str();
    for (U32 i = 0; i < _allFileTypes.size(); ++i) {
        if (_allFileTypes[i] == ext) {
            _filetypeCombo->setCurrentItem(i);
            break;
        }
    }
}
