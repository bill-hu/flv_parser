#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint8_t kTagTypeAudio = 8;
const uint8_t kTagTypeVideo = 9;
const uint8_t kTagTypeScript = 18;

FILE *logFp = NULL;
FILE *audioFp = NULL;
FILE *videoFp = NULL;

#pragma pack(1)

typedef struct {
    uint8_t signature[3];   // always FLV
    uint8_t version;        // file version
    uint8_t flags;          // video/audio tags present
    uint32_t dataOffset;    // the length of this header in bytes
} FlvHeader;

typedef struct {
    uint8_t tagType;        // 2bit - reserved  1bit - filter  5bit - tagType
    uint8_t dataSize[3];    // length of the message, from streamID to end of tag(equal to tag-11)
    uint8_t timestamp[3];   // time in milliseconds
    uint8_t timestampEx;    // extension of the timestamp field
    uint8_t streamID[3];    // always 0
} TagHeader;

#pragma pack(pop)

// forward declare
void parseMetaData(const uint8_t *metadata, uint32_t dataSize);

// read Big-Endian byte array into Little-Endian integer
uint32_t readBigEndian(const uint8_t *data, uint32_t length)
{
    int32_t result = 0;
    for (uint32_t i = 0; i < length; i++) {
        result |= (*(data + i) << ((length - 1 - i) * 8));
    }
    return result;
}

// string to double value
double stringToDouble(const uint8_t *hex, uint32_t length)
{
    double result = 0.0;
    char hexStr[length * 2];
    memset(hexStr, 0, sizeof(hexStr));
    for (uint32_t i = 0; i < length; i++) {
        sprintf(hexStr + i * 2, "%02x", hex[i]);
    }
    sscanf(hexStr, "%llx", &result);
    return result;
}

void parseScripteTag(const uint8_t *tagData, uint32_t dataSize)
{
    uint32_t offset = 0;
    uint8_t amfType = tagData[offset++];
    if (amfType == 0x02) {
        // AMF0 - onMetaData
        // 1Byte - AMF type, always 0x02
        // 2-3Byte - UINT16, always 0x000A,  length of 'onMetaData'
        // string - always 'onMetaData'
        uint16_t length = (uint16_t)readBigEndian(tagData+offset, 2);
        offset += 2;
        char *str = (char *)malloc(length + 1);
        memset(str, 0, length + 1);
        memcpy(str, tagData+offset, length);
        offset += length;
        // print AMF0 information
        fprintf(logFp, "AMF0 length=%d, str=%s\n", length, str);
        free(str);
    }
    else {
        fprintf(stderr, "parse script tag failed, AMF0 type != 0x02\n");
        return;
    }
    amfType = tagData[offset++];
    if (amfType == 0x08) {
        // AMF1 - array of metadata
        // 1Byte - AMF type, always 0x08
        // 2-5Byte - UINT32, size of metadata array
        // metadata
        uint32_t elemSize = readBigEndian(tagData+offset, 4);
        offset += 4;
        fprintf(logFp, "AMF1 element size=%d\n", elemSize);
        if (offset > dataSize) {
            fprintf(stderr, "metadata element is empty.\n");
            return;
        }
        parseMetaData(tagData+offset, dataSize-offset);
    }
    else {
        fprintf(stderr, "parse script tag failed, AMF1 type != 0x08.\n");
        return;
    }
}

void parseMetaData(const uint8_t *metadata, uint32_t dataSize)
{
    // 1-2Byte - length of element string, assume L
    // string with L length
    // L+3Byte - element type
    // value, size depends on element type
    uint32_t offset  = 0;   // read metadata offset

    bool boolValue = false;
    double doubleValue = 0.0;
    char *str = NULL;
    while (offset < dataSize) {
        // length
        uint16_t length = readBigEndian(metadata+offset, 2);
        offset += 2;

        // string
        char *element = (char *)malloc(length + 1);
        memset(element, 0, length + 1);
        memcpy(element, metadata+offset, length);
        offset += length;

        // type
        uint8_t type = metadata[offset++];
        switch (type) {
        case 0x0:   // number - 8 bytes
        {
            doubleValue = stringToDouble(metadata+offset, 8);
            offset += 8;
            fprintf(logFp, "metadata, numberType: element=%s, value=%f\n", element, doubleValue);

            break;
        }
        case 0x1:  // bool - 1 byte
        {
            boolValue = (bool)metadata[offset++];
            fprintf(logFp, "metadata, boolType: element=%s, value=%d\n", element, boolValue);

            break;
        }
        case 0x2:  // string
        {
            uint16_t len = (uint16_t)readBigEndian(metadata + offset, 2);
            offset += 2;
            str = (char *)malloc(len + 1);
            memset(str, 0, len + 1);
            memcpy(str, metadata + offset, len);
            offset += len;
            fprintf(logFp, "metadata, stringType: element=%s, value=%s\n", element, str);

            break;
        }
        case 0x12:  // long string
        {
            uint32_t len = readBigEndian(metadata + offset, 4);
            offset += 4;
            str = (char *)malloc(len + 1);
            memset(str, 0, len + 1);
            memcpy(str, metadata + offset, len);
            offset += len;
            fprintf(logFp, "metadata, longString: element=%s, value=%s\n", element, str);

            break;
        }
        default:
        {
            break;
        }
        }   // end of swith
    }
}

void parseAudioTag(const uint8_t *audioData, uint32_t dataSize)
{
    uint32_t offset = 0;
    // sound format - 4bit
    // sound rate - 2bit
    // sound size - 1bit
    // sound type = 1bit
    uint8_t firstbyte = audioData[offset++];
    uint8_t soundFormat = (firstbyte & 0xF0) >> 4;
    uint8_t soundRate = (firstbyte & 0x0C) >> 2;
    uint8_t soundSize = (firstbyte & 0x02) >> 1;
    uint8_t soundType = (firstbyte & 0x01);

    char outputStr[1024] = { 0 };
    switch (soundFormat) {
    case 0:
        strcat(outputStr, "Linear PCM, platform endian");
        break;
    case 1:
        strcat(outputStr, "ADPCM");
        break;
    case 2:
        strcat(outputStr, "MP3");
        break;
    case 3:
        strcat(outputStr, "Linear PCM, little endian");
        break;
    case 4:
        strcat(outputStr, "Nellymoser 16-kHz mono");
        break;
    case 5:
        strcat(outputStr, "Nellymoser 8-kHz mono");
        break;
    case 6:
        strcat(outputStr, "Nellymoser");
        break;
    case 7:
        strcat(outputStr, "G.711 A-law logarithmic PCM");
        break;
    case 8:
        strcat(outputStr, "G.711 mu-law logarithmic PCM");
        break;
    case 9:
        strcat(outputStr, "reserved");
        break;
    case 10:
        strcat(outputStr, "AAC");
        break;
    case 11:
        strcat(outputStr, "Speex");
        break;
    case 14:
        strcat(outputStr, "MP3 8-Khz");
        break;
    case 15:
        strcat(outputStr, "Device-specific sound");
        break;
    default:
        strcat(outputStr, "SoundFormat UNKNOWN");
        break;
    }
    strcat(outputStr, " | ");
    switch (soundRate) {
    case 0:
        strcat(outputStr, "5.5kHz");
        break;
    case 1:
        strcat(outputStr, "11kHz");
        break;
    case 2:
        strcat(outputStr, "22kHz");
        break;
    case 3:
        strcat(outputStr, "44kHz");
        break;
    }
    strcat(outputStr, " | ");
    switch (soundSize) {
    case 0:
        strcat(outputStr, "8-bit samples");
        break;
    case 1:
        strcat(outputStr, "16-bit samples");
        break;
    }
    strcat(outputStr, " | ");
    switch (soundType) {
    case 0:
        strcat(outputStr, "Mono sound");
        break;
    case 1:
        strcat(outputStr, "Stereo sound");
        break;
    }
    strcat(outputStr, " | ");
    if (soundFormat == 10) {
        uint8_t aacPacketType = audioData[offset++];
        switch(aacPacketType) {
        case 0:
            strcat(outputStr, "AAC sequence header");
            break;
        case 1:
            strcat(outputStr, "AAC raw");
            break;
        }
    }
    strcat(outputStr, "\n");
    fprintf(logFp, "%s", outputStr);
    // write audio data
    fwrite(audioData+offset, 1, dataSize-offset, audioFp);
}

void parseVideoTag(const uint8_t *videoData, uint32_t dataSize)
{
    // frame type - 4 bit
    // codecID - 4 bit
    uint8_t frameType = (videoData[0] & 0xF0) >> 4;
    uint8_t codecID = videoData[0] & 0x0F;
    char outputStr[1024] = { 0 };
    switch (frameType)
    {
    case 1:
        strcat(outputStr,"key frame  ");
        break;
    case 2:
        strcat(outputStr,"inter frame");
        break;
    case 3:
        strcat(outputStr,"disposable inter frame");
        break;
    case 4:
        strcat(outputStr,"generated keyframe");
        break;
    case 5:
        strcat(outputStr,"video info/command frame");
        break;
    default:
        strcat(outputStr,"frameType UNKNOWN");
        break;
    }
    strcat(outputStr,"| ");
    switch (codecID)
    {
    case 1:
        strcat(outputStr,"JPEG (currently unused)");
        break;
    case 2:
        strcat(outputStr,"Sorenson H.263");
        break;
    case 3:
        strcat(outputStr,"Screen video");
        break;
    case 4:
        strcat(outputStr,"On2 VP6");
        break;
    case 5:
        strcat(outputStr,"On2 VP6 with alpha channel");
        break;
    case 6:
        strcat(outputStr,"Screen video version 2");
        break;
    case 7:
        strcat(outputStr,"AVC");
        break;
    default:
        strcat(outputStr,"codecID UNKNOWN");
        break;
    }
    strcat(outputStr, "\n");
    fprintf(logFp, "%s", outputStr);

    // write video data to file
    fwrite(videoData, 1, dataSize, videoFp);
}

int parseFlv(const char *path)
{
    FILE *flvFp = fopen(path, "rb+");
    if (flvFp == NULL) {
        fprintf(stderr, "open flv file failed.\n");
        return -1;
    }

    // flv header
    FlvHeader flvHeader;
    fread(&flvHeader, 1, sizeof(flvHeader), flvFp);
    fprintf(logFp, "signature:  %c %c %c\n", flvHeader.signature[0], flvHeader.signature[1], flvHeader.signature[2]);
    fprintf(logFp, "version:    %x\n", flvHeader.version);
    fprintf(logFp, "flags:      %x\n", flvHeader.flags);
    uint32_t dataOffset = readBigEndian((uint8_t *)&flvHeader.dataOffset, sizeof(flvHeader.dataOffset));
    fprintf(logFp, "headersize: %d\n", dataOffset);

    uint32_t prevTagSize = 0;
    // write flv video data to file
    fwrite(&flvHeader, 1, sizeof(flvHeader), videoFp);
    fwrite(&prevTagSize, 1, sizeof(prevTagSize), videoFp); // prevTagSize=0

    // move the file pointer to the end of the header
    fseek(flvFp, dataOffset, SEEK_SET);

    TagHeader tagHeader;
    // parse tags
    do {
        // previous tag size
        uint8_t prevTagSizeTmp[4] = { 0 };
        fread(prevTagSizeTmp, 1, sizeof(prevTagSizeTmp), flvFp);
        prevTagSize = readBigEndian(prevTagSizeTmp, sizeof(prevTagSizeTmp));

        // tag header
        fread(&tagHeader, 1, sizeof(tagHeader), flvFp);
        uint32_t tagDataSize = readBigEndian(tagHeader.dataSize, sizeof(tagHeader.dataSize));
        uint32_t tagTimestamp = readBigEndian(tagHeader.timestamp, sizeof(tagHeader.timestamp));

        fprintf(logFp, "tagHeader | prevTagSize=%d | tagDataSize=%d | tagTimestamp=%d | ", prevTagSize, tagDataSize, tagTimestamp);

        uint8_t *tagData = (uint8_t *)malloc(tagDataSize);
        fread(tagData, 1, tagDataSize, flvFp);

        switch (tagHeader.tagType) {
        case kTagTypeAudio:
        {
            parseAudioTag(tagData, tagDataSize);
            break;
        }
        case kTagTypeVideo:
        {
            if (!feof(flvFp)) {
                // write video data
                fwrite(&tagHeader, 1, sizeof(tagHeader), videoFp);
                parseVideoTag(tagData, tagDataSize);
                // write previous tag size
                for (int byteSize = 0; byteSize < 4; byteSize++) {
                    fputc(fgetc(flvFp), videoFp);
                }
                fseek(flvFp, -4, SEEK_CUR);
            }
            break;
        }
        case kTagTypeScript:
        {
            parseScripteTag(tagData, tagDataSize);
            break;
        }
        default:
        {
            fprintf(stderr, "unknown tag type.\n");
            break;
        }
        free(tagData);
        }
    } while (!feof(flvFp));
    fclose(flvFp);

    return 0;
}

int main(int argc, char *argv[])
{
    logFp = fopen("./info.log", "wb");
    audioFp = fopen("./flv_audio.mp3", "wb");
    videoFp = fopen("./flv_video.flv", "wb");

    parseFlv("./cuc_ieschool.flv");

    fclose(logFp);
    fclose(audioFp);
    fclose(videoFp);

    return 0;
}
