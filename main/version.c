#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include "stateProbe.h"
#include "version.h"

/*static const char TAG[] = "version";*/
static const char versionProbeCmd[] = "version";
static stateProbe_probe versionProbe;

#ifdef CONFIG_IDF_TARGET_ESP8266
#include <esp_ota_ops.h>
#else
#include <esp_app_desc.h>
#endif

static void version_cb(const char* data, int length)
{
  (void)data; (void)length;

  /* Buffer for the response */
  char buf[256];
  int n = 0;

#ifdef CONFIG_IDF_TARGET_ESP8266
  const esp_app_desc_t *desc = esp_ota_get_app_description();
  if (desc) {
    n = snprintf(buf, sizeof(buf), "name:%s\nversion:%s\ndate:%s\n", desc->project_name, desc->version, desc->date);
  } else {
    n = snprintf(buf, sizeof(buf), "no app description available\n");
  }
#else
  const esp_app_desc_t *desc = esp_app_get_description();
  if (desc) {
    n = snprintf(buf, sizeof(buf), "name:%s\nversion:%s\ndate:%s\n", desc->project_name, desc->version, desc->date);
  } else {
    n = snprintf(buf, sizeof(buf), "no app description available\n");
  }
#endif

  if (n < 0) n = 0;
  stateProbe_write(&versionProbe, buf, n);
}

void version_init(void)
{
  stateProbe_register(&versionProbe, versionProbeCmd, version_cb);
}
