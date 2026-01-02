#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "esp_log.h"

#include "i2cdev.h"

#define TAG "I2CDEV"

esp_err_t i2c_dev_init(i2c_dev_t *dev)
{
	ESP_LOGI(TAG, "Using shared I2C bus from sensor manager");
	
	// Use shared I2C bus handle from sensor manager if available
	if (dev->_i2c_bus_handle != NULL) {
		ESP_LOGI(TAG, "Using provided shared I2C bus handle");
	} else {
		ESP_LOGI(TAG, "Creating new I2C bus (fallback mode)");
		i2c_master_bus_config_t i2c_mst_config = {
			.clk_source = I2C_CLK_SRC_DEFAULT,
			.glitch_ignore_cnt = 7,
			.i2c_port = dev->port,
			.scl_io_num = dev->scl_io_num,
			.sda_io_num = dev->sda_io_num,
			.flags.enable_internal_pullup = true,
		};
		i2c_master_bus_handle_t i2c_bus_handle;
		esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
			return ret;
		}
		dev->_i2c_bus_handle = i2c_bus_handle;
	}

	i2c_device_config_t dev_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = dev->addr,
		.scl_speed_hz = dev->clk_speed,
	};
	i2c_master_dev_handle_t i2c_dev_handle;
	esp_err_t ret = i2c_master_bus_add_device(dev->_i2c_bus_handle, &dev_cfg, &i2c_dev_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
		return ret;
	}

	dev->_i2c_dev_handle = i2c_dev_handle;
	return ESP_OK;
}

esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
	if (!dev || !in_data || !in_size) return ESP_ERR_INVALID_ARG;

	if (out_data && out_size) {
		ESP_ERROR_CHECK(i2c_master_transmit_receive(dev->_i2c_dev_handle, out_data, out_size, in_data, in_size, I2CDEV_TIMEOUT));
	} else {
		ESP_ERROR_CHECK(i2c_master_receive(dev->_i2c_dev_handle, in_data, in_size, I2CDEV_TIMEOUT));
	}
	return ESP_OK;
}

esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size)
{
	if (!dev || !out_data || !out_size) return ESP_ERR_INVALID_ARG;

	uint8_t *out_buf = malloc(out_reg_size+out_size);
	if (out_buf == NULL) {
		ESP_LOGE(__FUNCTION__, "Error allocating memory for out_buf");
		return ESP_FAIL;
	}
	int out_index = 0;
	if (out_reg && out_reg_size) {
		memcpy(out_buf, out_reg, out_reg_size);
		out_index = out_index + out_reg_size;
	}
	memcpy(&out_buf[out_index], out_data, out_size);
	out_index = out_index + out_size;
	ESP_ERROR_CHECK(i2c_master_transmit(dev->_i2c_dev_handle, out_buf, out_index, I2CDEV_TIMEOUT));
	free(out_buf);
	return ESP_OK;
}