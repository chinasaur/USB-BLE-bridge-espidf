
/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <catch2/catch_test_macros.hpp>

#include "usb/cdc_acm_host.h"
#include "esp_system.h"

extern "C" {
#include "Mockusb_host.h"
#include "Mockqueue.h"
#include "Mocktask.h"
#include "Mockidf_additions.h"
#include "Mockportmacro.h"
#include "Mockevent_groups.h"
}

int task_handle_ptr;
// Ensure task_handle is non nullptr, as it is used in ReturnThruPtr type of Cmock function
TaskHandle_t task_handle = reinterpret_cast<TaskHandle_t>(&task_handle_ptr);
int sem;
int event_group;


SCENARIO("CDC-ACM Host install/uninstall")
{
    // CDC-ACM Host driver config set to nullptr
    GIVEN("NO CDC-ACM Host driver config, driver not installed") {

        // Try to uninstall CDC ACM Host (not previously installed)
        // Expect fail because of p_cdc_acm_obj being nullptr
        SECTION("Try to uninstall not installed CDC ACM Host") {

            vPortEnterCritical_Expect();
            vPortExitCritical_Expect();

            // Call the DUT function, expect ESP_ERR_INVALID_STATE
            REQUIRE(ESP_ERR_INVALID_STATE == cdc_acm_host_uninstall());
        }

        // Try to install CDC ACM Host, induce an error during install
        // Expect fail to create EventGroup
        SECTION("Fail to create EventGroup") {

            // Create an EventGroup, return nullptr, so the EventGroup is not created successfully
            xEventGroupCreate_ExpectAndReturn(nullptr);
            // We should be calling xSemaphoreCreteMutex_ExpectAnyArgsAndRetrun instead of xQueueCreateMutex_ExpectAnyArgsAndReturn
            // Because of missing Freertos Mocks
            // Create a semaphore, return the semaphore handle (Queue Handle in this scenario), so the semaphore is created successfully
            xQueueCreateMutex_ExpectAnyArgsAndReturn(reinterpret_cast<QueueHandle_t>(&sem));
            // Create a task, return pdTRUE, so the task is created successfully
            xTaskCreatePinnedToCore_ExpectAnyArgsAndReturn(pdTRUE);
            // Return task handle by pointer
            xTaskCreatePinnedToCore_ReturnThruPtr_pxCreatedTask(&task_handle);

            // goto err: (xEventGroupCreate returned nullptr), delete the queue and the task
            vQueueDelete_Expect(reinterpret_cast<QueueHandle_t>(&sem));
            vTaskDelete_Expect(task_handle);

            // Call the DUT function, expect ESP_ERR_NO_MEM
            REQUIRE(ESP_ERR_NO_MEM == cdc_acm_host_install(nullptr));
        }

        // Install CDC ACM Host
        // Expect to successfully install the CDC ACM host
        SECTION("Successfully install CDC ACM Host") {

            // Create an EventGroup, return event group handle, so the EventGroup is created successfully
            xEventGroupCreate_ExpectAndReturn(reinterpret_cast<EventGroupHandle_t>(&event_group));
            // We should be calling xSemaphoreCreteMutex_ExpectAnyArgsAndRetrun instead of xQueueCreateMutex_ExpectAnyArgsAndReturn
            // Because of missing Freertos Mocks
            // Create a semaphore, return the semaphore handle (Queue Handle in this scenario), so the semaphore is created successfully
            xQueueCreateMutex_ExpectAnyArgsAndReturn(reinterpret_cast<QueueHandle_t>(&sem));
            // Create a task, return pdTRUE, so the task is created successfully
            vPortEnterCritical_Expect();
            vPortExitCritical_Expect();
            xTaskCreatePinnedToCore_ExpectAnyArgsAndReturn(pdTRUE);
            // Return task handle by pointer
            xTaskCreatePinnedToCore_ReturnThruPtr_pxCreatedTask(&task_handle);

            // Call mocked function from USB Host
            // return ESP_OK, so the client si registered successfully
            usb_host_client_register_ExpectAnyArgsAndReturn(ESP_OK);

            // Resume the task
            xTaskGenericNotify_ExpectAnyArgsAndReturn(pdPASS);

            // Call the DUT Function, expect ESP_OK
            REQUIRE(ESP_OK == cdc_acm_host_install(nullptr));
        }
    }

    GIVEN("NO CDC-ACM Host driver config, driver already installed") {

        // Try to install CDC ACM Host again, while it is already installed (from the previous test section)
        // Expect fail because of p_cdc_acm_obj being non nullptr
        SECTION("Try to install CDC ACM Host again") {

            // Call the DUT function, expect ESP_ERR_INVALID_STATE
            REQUIRE(ESP_ERR_INVALID_STATE == cdc_acm_host_install(nullptr));
        }

        // Uninstall CDC ACM Host
        // Expect to successfully uninstall the CDC ACM Host
        SECTION("Successfully uninstall CDC ACM Host") {

            vPortEnterCritical_Expect();
            vPortExitCritical_Expect();
            xQueueSemaphoreTake_ExpectAndReturn(reinterpret_cast<QueueHandle_t>(&sem), portMAX_DELAY, pdTRUE);
            vPortEnterCritical_Expect();
            vPortExitCritical_Expect();

            // Signal to CDC task to stop, unblock it and wait for its deletion
            xEventGroupSetBits_ExpectAndReturn(reinterpret_cast<EventGroupHandle_t>(&event_group), BIT0, pdTRUE);

            // Call mocked function from USB Host
            // return ESP_OK, so the client si unblocked successfully
            usb_host_client_unblock_ExpectAnyArgsAndReturn(ESP_OK);
            xEventGroupWaitBits_ExpectAndReturn(reinterpret_cast<EventGroupHandle_t>(&event_group), BIT1, pdFALSE, pdFALSE, pdMS_TO_TICKS(100), pdTRUE);

            // Free remaining resources
            vEventGroupDelete_Expect(reinterpret_cast<EventGroupHandle_t>(&event_group));
            xQueueGenericSend_ExpectAnyArgsAndReturn(pdTRUE);
            vQueueDelete_Expect(reinterpret_cast<QueueHandle_t>(&sem));

            // Call the DUT function, expect ESP_OK
            REQUIRE(ESP_OK == cdc_acm_host_uninstall());
        }
    }
}
