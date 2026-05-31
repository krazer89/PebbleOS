/* SPDX-License-Identifier: Apache-2.0 */

#include "pbl/services/speaker/pcm_hour_beep.h"
#include "pbl/services/speaker/speaker_service.h"
#include "resource/resource.h"
#include "resource/resource_ids.auto.h"

#include "kernel/util/sleep.h"

#ifdef CONFIG_SPEAKER

static const size_t HOUR_BEEP_CHUNK_SIZE = 1024;
static const uint8_t HOUR_BEEP_VOLUME = 20;
static const uint32_t HOUR_BEEP_RETRY_DELAY_MS = 1;

static const uint8_t *s_hour_beep_pcm = NULL;
static size_t s_hour_beep_pcm_size = 0;

static bool prv_fill_stream(const uint8_t *data, size_t total_bytes, size_t *offset) {
  while (*offset < total_bytes) {
    size_t remaining = total_bytes - *offset;
    size_t to_write = (remaining > HOUR_BEEP_CHUNK_SIZE) ? HOUR_BEEP_CHUNK_SIZE : remaining;
    uint32_t written = speaker_service_stream_write(data + *offset, (uint32_t)to_write);

    if (written == 0) {
      return false;
    }

    *offset += written;
  }

  return true;
}

bool speaker_service_play_hour_beep(void) {
  if (s_hour_beep_pcm == NULL) {
    size_t num_bytes = 0;
    const uint8_t *data =
        resource_get_readonly_bytes(SYSTEM_APP, RESOURCE_ID_HOUR_BEEP_PCM, &num_bytes, true);
    if (data == NULL || num_bytes == 0 || (num_bytes % sizeof(int16_t)) != 0) {
      return false;
    }
    s_hour_beep_pcm = data;
    s_hour_beep_pcm_size = num_bytes;
  }

  if (!speaker_service_stream_open(SpeakerPriorityNotification, HOUR_BEEP_VOLUME,
                                   SpeakerPcmFormat_16kHz_16bit)) {
    return false;
  }

  size_t offset = 0;
  while (!prv_fill_stream(s_hour_beep_pcm, s_hour_beep_pcm_size, &offset)) {
    psleep(HOUR_BEEP_RETRY_DELAY_MS);
  }

  speaker_service_stream_close();
  return true;
}
#endif
