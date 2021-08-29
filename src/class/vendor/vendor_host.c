/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_VENDOR)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "host/usbh.h"
#include "vendor_host.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
custom_interface_info_t custom_interface[CFG_TUSB_HOST_DEVICE_MAX];

static tusb_error_t cush_validate_paras(uint8_t dev_addr, uint16_t vendor_id, uint16_t product_id, const void * p_buffer, uint16_t length)
{
  if ( !tusbh_custom_is_mounted(dev_addr, vendor_id, product_id) )
  {
    return TUSB_ERROR_DEVICE_NOT_READY;
  }

  TU_ASSERT( p_buffer != NULL && length != 0, TUSB_ERROR_INVALID_PARA);

  return TUSB_ERROR_NONE;
}
//--------------------------------------------------------------------+
// APPLICATION API (need to check parameters)
//--------------------------------------------------------------------+
bool tusbh_custom_is_mounted(uint8_t dev_addr, uint16_t vendor_id, uint16_t product_id)
{
  custom_interface_info_t *itf = &custom_interface[dev_addr-1];

  return itf->vendor_id == vendor_id && itf->product_id == product_id && itf->ep_in && itf->ep_out;
}

bool tusbh_custom_read(uint8_t dev_addr, uint16_t vendor_id, uint16_t product_id, void * p_buffer, uint16_t length)
{
  TU_ASSERT( cush_validate_paras(dev_addr, vendor_id, product_id, p_buffer, length) );

  TU_VERIFY( usbh_edpt_claim(dev_addr, custom_interface[dev_addr-1].ep_in) );

  return usbh_edpt_xfer( dev_addr, custom_interface[dev_addr-1].ep_in, p_buffer, length);
}

bool tusbh_custom_write(uint8_t dev_addr, uint16_t vendor_id, uint16_t product_id, void const * p_data, uint16_t length)
{
  TU_ASSERT( cush_validate_paras(dev_addr, vendor_id, product_id, p_data, length) );

  TU_VERIFY( usbh_edpt_claim(dev_addr, custom_interface[dev_addr-1].ep_out) );

  return usbh_edpt_xfer( dev_addr, custom_interface[dev_addr-1].ep_out, (void *) p_data, length);
}

//--------------------------------------------------------------------+
// USBH-CLASS API
//--------------------------------------------------------------------+
void cush_init(void)
{
  tu_memclr(&custom_interface, sizeof(custom_interface_info_t) * CFG_TUSB_HOST_DEVICE_MAX);
}

bool cush_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *p_interface_desc, uint16_t *p_length)
{
  // FIXME quick hack to test lpc1k custom class with 2 bulk endpoints
  uint8_t const *p_desc = (uint8_t const *) p_interface_desc;
  p_desc = tu_desc_next(p_desc);

  tuh_vid_pid_get(dev_addr, &custom_interface[dev_addr-1].vendor_id, &custom_interface[dev_addr-1].product_id);

  //------------- Bulk Endpoints Descriptor -------------//
  for(uint32_t i=0; i<2; i++)
  {
    tusb_desc_endpoint_t const *p_endpoint = (tusb_desc_endpoint_t const *) p_desc;
    TU_ASSERT(TUSB_DESC_ENDPOINT == p_endpoint->bDescriptorType, TUSB_ERROR_INVALID_PARA);

    uint8_t * p_ep_addr =  ( p_endpoint->bEndpointAddress &  TUSB_DIR_IN_MASK ) ?
                         &custom_interface[dev_addr-1].ep_in : &custom_interface[dev_addr-1].ep_out;

    *p_ep_addr = p_endpoint->bEndpointAddress;

    TU_ASSERT( usbh_edpt_open(rhport, dev_addr, p_endpoint) );

    p_desc = tu_desc_next(p_desc);
  }

  (*p_length) = sizeof(tusb_desc_interface_t) + 2*sizeof(tusb_desc_endpoint_t);

  return true;
}

bool cush_set_config(uint8_t dev_addr, uint8_t itf_num)
{
  usbh_driver_set_config_complete(dev_addr, itf_num);
  return true;
}

bool cush_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes)
{
  (void) dev_addr; (void) ep_addr; (void) event; (void) xferred_bytes;
  return true;
}

void cush_close(uint8_t dev_addr)
{
  custom_interface_info_t * p_interface = &custom_interface[dev_addr-1];

  tu_memclr(p_interface, sizeof(custom_interface_info_t));
}

#endif
