# flv_parser

a simple flv parser imported from https://github.com/longlibj8355/flv_parser

usage: flv_parser flvname

outuput: info.log , a text file with flv file info,flv_video.flv and flv_audio.mp3

example of info.log:

signature:  F L V
version:    1
flags:      5
headersize: 9
tagHeader | prevTagSize=0 | tagDataSize=551 | tagTimestamp=0 | AMF0 length=10, str=onMetaData
AMF1 element size=25
metadata, numberType: element=duration, value=34.158000
metadata, numberType: element=width, value=512.000000
metadata, numberType: element=height, value=288.000000
metadata, numberType: element=videodatarate, value=179.284180
metadata, numberType: element=framerate, value=1000.000000
metadata, numberType: element=videocodecid, value=7.000000
metadata, numberType: element=audiodatarate, value=0.000000
metadata, numberType: element=audiosamplerate, value=44100.000000
metadata, numberType: element=audiosamplesize, value=16.000000
metadata, boolType: element=stereo, value=1
metadata, numberType: element=audiocodecid, value=2.000000
metadata, stringType: element=metadatacreator, value=iku
metadata, stringType: element=hasKeyframes, value=true
metadata, stringType: element=hasVideo, value=true
metadata, stringType: element=hasAudio, value=true
metadata, stringType: element=hasMetadata, value=true
metadata, stringType: element=canSeekToEnd, value=false
metadata, stringType: element=datasize, value=932906
metadata, stringType: element=videosize, value=787866
metadata, stringType: element=audiosize, value=140052
metadata, stringType: element=lasttimestamp, value=34
metadata, stringType: element=lastkeyframetimestamp, value=30
metadata, stringType: element=lastkeyframelocation, value=886498
metadata, stringType: element=encoder, value=Lavf55.19.104
metadata, numberType: element=filesize, value=1358127.000000
tagHeader | prevTagSize=562 | tagDataSize=45 | tagTimestamp=0 | key frame  | AVC
tagHeader | prevTagSize=56 | tagDataSize=418 | tagTimestamp=0 | MP3 | 44kHz | 16-bit samples | Stereo sound | 
tagHeader | prevTagSize=429 | tagDataSize=2463 | tagTimestamp=25 | key frame  | AVC
tagHeader | prevTagSize=2474 | tagDataSize=419 | tagTimestamp=26 | MP3 | 44kHz | 16-bit samples | Stereo sound |
