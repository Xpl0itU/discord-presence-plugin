#include <wups.h>

#include <cstdlib>
#include <cstring>

#include <nn/acp/title.h>

#include <curl/curl.h>

WUPS_PLUGIN_NAME("discordPresencePlugin");
WUPS_PLUGIN_DESCRIPTION(
    "Sends your current playing game to your discord status");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("DaThinkingChair");
WUPS_PLUGIN_LICENSE("GPLv3");

uint64_t titleID;
char token[72];

int setDiscordStatus(const char *token, const char *playingTitle) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://discord.com/api/v8/users/@me/settings");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    char fields[1024];
    sprintf(fields, "{\"custom_status\":{\"text\":\"Playing "
                     "%s\",\"emoji_id\":null,\"emoji_name\":null}}", playingTitle);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);

    char auth_header[100];
    sprintf(auth_header, "Authorization: %s", token);
    headers = curl_slist_append(headers, auth_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  return 0;
}

INITIALIZE_PLUGIN() {
  FILE *fp = fopen("fs:/vol/external01/wiiu/discordToken.txt", "r");
  if (!fp)
    return;
  fread(token, 71, 1, fp);
  fclose(fp);
}

DECL_FUNCTION(uint32_t, MCP_RightCheckLaunchable, uint32_t *u1, uint32_t *u2,
              uint64_t u3, uint32_t u4) {
  uint32_t result = real_MCP_RightCheckLaunchable(u1, u2, u3, u4);

  if (result == 0 && strlen(token) != 0) {
    titleID = u3;
  }

  return result;
}

ON_APPLICATION_REQUESTS_EXIT() {
  if (strlen(token) == 0) {
    return;
  } else if (titleID == 0) {
    return;
  }
  // need to be aligned to 0x40 for IOSU
  auto *meta = (ACPMetaXml *)aligned_alloc(0x40, sizeof(ACPMetaXml));

  if (!meta) {
    return;
  }

  ACPResult result = ACPGetTitleMetaXml(titleID, meta);
  if (result == ACPResult::ACP_RESULT_SUCCESS) {
    setDiscordStatus(token, meta->shortname_en);
  }

  free(meta);

  titleID = 0;
}

WUPS_MUST_REPLACE(MCP_RightCheckLaunchable, WUPS_LOADER_LIBRARY_COREINIT,
                  MCP_RightCheckLaunchable);