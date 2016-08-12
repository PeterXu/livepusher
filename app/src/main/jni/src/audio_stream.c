#include "common.h"

void fireAudio(audio_enc_t *audio, uint8_t* data, int len) {
    if (!audio || !audio->handle) {
        LOGE("[%s] invalid audio handle", __FUNCTION__);
        return;
    }

    unsigned char * bitbuf = (unsigned char*) malloc(audio->nMaxOutputBytes * sizeof(unsigned char));
    int byteslen = faacEncEncode(audio->handle, (int32_t *) data,
            audio->nInputSamples, bitbuf, audio->nMaxOutputBytes);
    if (byteslen > 0) {
        add_aac_body(bitbuf, byteslen);
    }
    free(bitbuf);
}

void releaseAudio(audio_enc_t *audio) {
    if (audio && audio->handle) {
        faacEncClose(audio->handle);
        audio->handle = NULL;
    }
}

int setAudioOptions(audio_enc_t *audio, int sampleRate, int channel, int bitrate) {
    if (audio && audio->handle) {
        LOGD("[%s] encoder has been opened", __FUNCTION__);
        return 0;
    }

    LOGI("[%s] options: %d, %d, %d", __FUNCTION__, sampleRate, channel, bitrate);
    audio->handle = faacEncOpen(sampleRate, channel, &audio->nInputSamples, &audio->nMaxOutputBytes);
    if (!audio->handle) {
        LOGD("[%s] fail to faacEncOpen", __FUNCTION__);
        notifyNativeInfo(LOG_ERROR, E_AUDIO_ENCODER);
        return -1;
    }

    faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(audio->handle);
    pConfiguration->mpegVersion = MPEG4;
    pConfiguration->allowMidside = 1;
    pConfiguration->aacObjectType = LOW;
    pConfiguration->outputFormat = 0;
    pConfiguration->useTns = 1;
    pConfiguration->useLfe = 0;
    pConfiguration->bitRate = bitrate / channel;
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
    pConfiguration->quantqual = 100;
    pConfiguration->bandWidth = 0;
    pConfiguration->shortctl = SHORTCTL_NORMAL;

    int ret = faacEncSetConfiguration(audio->handle, pConfiguration);
    LOGI("[%s] encoder configration, ret=%d", __FUNCTION__, ret);
    return ret;
}

