#include "https_request.h"

/* HTTPS task handler */

TaskHandle_t get_task_handler = NULL;
TaskHandle_t patch_task_handler = NULL;

static const char *TAG = "example";

/* Timer interval once every day (24 Hours) */
#define TIME_PERIOD (86400000000ULL)

static const char GET_REQUEST[] = "GET " GET_WEB_URL " HTTP/1.1\r\n"
                             "Host: "WEB_SERVER"\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "\r\n";

static const char *PATCH_DATA = "{\"temperature\": 23, \"humidity\": 60}";

static const char *PATCH_REQUEST_TEMPLATE =
                    "PATCH " WEB_URL " HTTP/1.1\r\n"
                    "Host: "WEB_SERVER"\r\n"
                    "User-Agent: esp-idf/1.0 esp32\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s";

#ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
static const char LOCAL_SRV_REQUEST[] = "GET " CONFIG_EXAMPLE_LOCAL_SERVER_URL " HTTP/1.1\r\n"
                             "Host: "WEB_SERVER"\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "\r\n";
#endif

/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

extern const uint8_t local_server_cert_pem_start[] asm("_binary_local_server_cert_pem_start");
extern const uint8_t local_server_cert_pem_end[]   asm("_binary_local_server_cert_pem_end");

#ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
static esp_tls_client_session_t *tls_client_session = NULL;
static bool save_client_session = false;
#endif

static void https_get_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *REQUEST)
{
    while(1) {
        ESP_LOGI(TAG, "This is get request !");

        char buf[1024];
        int ret, len;

        esp_tls_t *tls = esp_tls_init();
        if (!tls) {
            ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
            goto exit;
        }

        if (esp_tls_conn_http_new_sync(WEB_SERVER_URL, &cfg, tls) == 1) {
            ESP_LOGI(TAG, "Connection established...");
        } else {
            ESP_LOGE(TAG, "Connection failed...");
            goto cleanup;
        }

        #ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
            /* The TLS session is successfully established, now saving the session ctx for reuse */
            if (save_client_session) {
                esp_tls_free_client_session(tls_client_session);
                tls_client_session = esp_tls_get_client_session(tls);
            }
        #endif

        size_t written_bytes = 0;
        do {
            ret = esp_tls_conn_write(tls,
                                    REQUEST + written_bytes,
                                    strlen(REQUEST) - written_bytes);
            if (ret >= 0) {
                ESP_LOGI(TAG, "%d bytes written", ret);
                written_bytes += ret;
            } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
                ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
                goto cleanup;
            }
        } while (written_bytes < strlen(REQUEST));
        
        /* Reading http response */

        ESP_LOGI(TAG, "Reading HTTP response...");

        len = sizeof(buf) - 1;
        memset(buf, 0x00, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);
        ESP_LOGI(TAG, "len of response: %d", len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        } else if (ret < 0) {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        } else if (ret == 0) {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        for (int i = 0; i < len; i++) {
            putchar(buf[i]);
        }
        putchar('\n'); // JSON output doesn't have a newline at end

        cleanup:
            esp_tls_conn_destroy(tls);
        exit:
            // for (int countdown = 10; countdown >= 0; countdown--) {
            //     ESP_LOGI(TAG, "%d...", countdown);
            //     vTaskDelay(1000 / portTICK_PERIOD_MS);
            // }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void https_patch_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *PATCH_DATA)
{
    while (1)
    {
        char buf[512];
        int ret, len;
        ESP_LOGI(TAG, "This is patch request !");

        esp_tls_t *tls = esp_tls_init();
        if (!tls)
        {
            ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
            goto exit;
        }

        if (esp_tls_conn_http_new_sync(WEB_SERVER_URL, &cfg, tls) == 1)
        {
            ESP_LOGI(TAG, "Connection established...");
        }
        else
        {
            ESP_LOGE(TAG, "Connection failed...");
            goto cleanup;
        }

#ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
        /* The TLS session is successfully established, now saving the session ctx for reuse */
        if (save_client_session)
        {
            esp_tls_free_client_session(tls_client_session);
            tls_client_session = esp_tls_get_client_session(tls);
        }
#endif

        int content_length = snprintf(NULL, 0, PATCH_DATA);
        char patch_request[512];
        snprintf(patch_request, sizeof(patch_request), PATCH_REQUEST_TEMPLATE, content_length, PATCH_DATA);

        ESP_LOGE(TAG, "%s", patch_request);


        size_t written_bytes = 0;
        do
        {
            ret = esp_tls_conn_write(tls,
                                     patch_request + written_bytes,
                                     strlen(patch_request) - written_bytes);
            if (ret >= 0)
            {
                ESP_LOGI(TAG, "%d bytes written", ret);
                written_bytes += ret;
            }
            else if (ret != ESP_TLS_ERR_SSL_WANT_READ && ret != ESP_TLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
                // free(patch_request);  // Free allocated memory
                goto cleanup;
            }
        } while (written_bytes < strlen(patch_request));

        /* Reading http response */
        ESP_LOGI(TAG, "Reading HTTP response...");

        len = sizeof(buf) - 1;
        memset(buf, 0x00, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE || ret == ESP_TLS_ERR_SSL_WANT_READ)
        {
            free(patch_request);  // Free allocated memory
            continue;
        }
        else if (ret < 0)
        {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            free(patch_request);  // Free allocated memory
            break;
        }
        else if (ret == 0)
        {
            ESP_LOGI(TAG, "connection closed");
            free(patch_request);  // Free allocated memory
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        for (int i = 0; i < len; i++)
        {
            putchar(buf[i]);
        }
        putchar('\n'); // JSON output doesn't have a newline at the end

    cleanup:
        // free(patch_request);  // Free allocated memory
        esp_tls_conn_destroy(tls);
    exit:
        // No need for the countdown loop and delay here
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Before vTaskSuspend");
        vTaskSuspend(patch_task_handler);
        ESP_LOGI(TAG, "After vTaskSuspend");    }
}

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
static void https_get_request_using_crt_bundle(void)
{
    ESP_LOGI(TAG, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    https_get_request(cfg, GET_WEB_URL, GET_REQUEST);
}

static void https_patch_request_using_crt_bundle(void)
{
    ESP_LOGI(TAG, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    https_patch_request(cfg, WEB_URL, PATCH_DATA);
}

#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

static void https_get_request_using_cacert_buf(void)
{
    ESP_LOGI(TAG, "https_request using cacert_buf");
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *) server_root_cert_pem_start,
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };
    https_get_request(cfg, GET_WEB_URL, GET_REQUEST);
}

static void https_patch_request_using_cacert_buf(void)
{
    ESP_LOGI(TAG, "https_request using cacert_buf");
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *) server_root_cert_pem_start,
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };
    https_patch_request(cfg, WEB_URL, PATCH_DATA);
}

static void https_get_request_using_global_ca_store(void)
{
    esp_err_t esp_ret = ESP_FAIL;
    ESP_LOGI(TAG, "https_request using global ca_store");
    esp_ret = esp_tls_set_global_ca_store(server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in setting the global ca store: [%02X] (%s),could not complete the https_request using global_ca_store", esp_ret, esp_err_to_name(esp_ret));
        return;
    }
    esp_tls_cfg_t cfg = {
        .use_global_ca_store = true,
    };
    https_get_request(cfg, GET_WEB_URL, GET_REQUEST);
    esp_tls_free_global_ca_store();
}

static void https_patch_request_using_global_ca_store(void)
{
    esp_err_t esp_ret = ESP_FAIL;
    ESP_LOGI(TAG, "https_request using global ca_store");
    esp_ret = esp_tls_set_global_ca_store(server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in setting the global ca store: [%02X] (%s),could not complete the https_request using global_ca_store", esp_ret, esp_err_to_name(esp_ret));
        return;
    }
    esp_tls_cfg_t cfg = {
        .use_global_ca_store = true,
    };
    https_patch_request(cfg, WEB_URL, PATCH_DATA);
    esp_tls_free_global_ca_store();
}

#ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
static void https_get_request_to_local_server(const char* url)
{
    ESP_LOGI(TAG, "https_request to local server");
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *) local_server_cert_pem_start,
        .cacert_bytes = local_server_cert_pem_end - local_server_cert_pem_start,
        .skip_common_name = true,
    };
    save_client_session = true;
    https_get_request(cfg, url, LOCAL_SRV_REQUEST);
}

static void https_patch_request_to_local_server(const char* url)
{
    ESP_LOGI(TAG, "https_request to local server");
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *) local_server_cert_pem_start,
        .cacert_bytes = local_server_cert_pem_end - local_server_cert_pem_start,
        .skip_common_name = true,
    };
    save_client_session = true;
    https_patch_request(cfg, url, LOCAL_SRV_REQUEST);
}

static void https_get_request_using_already_saved_session(const char *url)
{
    ESP_LOGI(TAG, "https_request using saved client session");
    esp_tls_cfg_t cfg = {
        .client_session = tls_client_session,
    };
    https_get_request(cfg, url, LOCAL_SRV_REQUEST);
    esp_tls_free_client_session(tls_client_session);
    save_client_session = false;
    tls_client_session = NULL;
}
#endif

void https_request_task(void *pvparameters)
{
    ESP_LOGI(TAG, "Start https_request example");

    #ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
        char *server_url = NULL;
    #ifdef CONFIG_EXAMPLE_LOCAL_SERVER_URL_FROM_STDIN
        char url_buf[SERVER_URL_MAX_SZ];
        if (strcmp(CONFIG_EXAMPLE_LOCAL_SERVER_URL, "FROM_STDIN") == 0) {
            example_configure_stdin_stdout();
            fgets(url_buf, SERVER_URL_MAX_SZ, stdin);
            int len = strlen(url_buf);
            url_buf[len - 1] = '\0';
            server_url = url_buf;
        } else {
            ESP_LOGE(TAG, "Configuration mismatch: invalid url for local server");
            abort();
        }
        printf("\nServer URL obtained is %s\n", url_buf);
    #else
        server_url = CONFIG_EXAMPLE_LOCAL_SERVER_URL;
    #endif /* CONFIG_EXAMPLE_LOCAL_SERVER_URL_FROM_STDIN */
        https_get_request_to_local_server(server_url);
        https_get_request_using_already_saved_session(server_url);
    #endif

    #if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        ESP_LOGI(TAG, "first");

        https_get_request_using_crt_bundle();

        ESP_LOGI(TAG, "after");

    #endif
    ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
    https_get_request_using_cacert_buf();
    https_get_request_using_global_ca_store();
    ESP_LOGI(TAG, "Get again https_request example");
}

void https_patch_task(void *pvparameters)
{
    ESP_LOGI(TAG, "Start patch request task");

    #ifdef CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS
        char *server_url = NULL;
    #ifdef CONFIG_EXAMPLE_LOCAL_SERVER_URL_FROM_STDIN
        char url_buf[SERVER_URL_MAX_SZ];
        if (strcmp(CONFIG_EXAMPLE_LOCAL_SERVER_URL, "FROM_STDIN") == 0) {
            example_configure_stdin_stdout();
            fgets(url_buf, SERVER_URL_MAX_SZ, stdin);
            int len = strlen(url_buf);
            url_buf[len - 1] = '\0';
            server_url = url_buf;
        } else {
            ESP_LOGE(TAG, "Configuration mismatch: invalid url for local server");
            abort();
        }
        printf("\nServer URL obtained is %s\n", url_buf);
    #else
        server_url = CONFIG_EXAMPLE_LOCAL_SERVER_URL;
    #endif /* CONFIG_EXAMPLE_LOCAL_SERVER_URL_FROM_STDIN */
        https_get_request_to_local_server(server_url);
        https_get_request_using_already_saved_session(server_url);
    #endif

    #if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        ESP_LOGI(TAG, "first");

        https_patch_request_using_crt_bundle();

        ESP_LOGI(TAG, "after");

    #endif
    ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
    https_patch_request_using_cacert_buf();
    https_patch_request_using_global_ca_store();
    ESP_LOGI(TAG, "patch again https_request example");

}
