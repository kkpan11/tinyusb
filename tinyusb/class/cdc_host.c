/**************************************************************************/
/*!
    @file     cdc_host.c
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, hathach (tinyusb.org)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    This file is part of the tinyusb stack.
*/
/**************************************************************************/

#include "tusb_option.h"

#if (MODE_HOST_SUPPORTED && TUSB_CFG_HOST_CDC)

#define _TINY_USB_SOURCE_FILE_

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "common/common.h"
#include "cdc_host.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
/*STATIC_*/ cdch_data_t cdch_data[TUSB_CFG_HOST_DEVICE_MAX];

//--------------------------------------------------------------------+
// USBH-CLASS DRIVER API
//--------------------------------------------------------------------+
void cdch_init(void)
{
  memclr_(cdch_data, sizeof(cdch_data_t)*TUSB_CFG_HOST_DEVICE_MAX);
}

tusb_error_t cdch_open_subtask(uint8_t dev_addr, tusb_descriptor_interface_t const *p_interface_desc, uint16_t *p_length)
{

  if ( CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL != p_interface_desc->bInterfaceSubClass)
  {
    return TUSB_ERROR_CDCH_UNSUPPORTED_SUBCLASS;
  }

  if (CDC_COMM_PROTOCOL_ATCOMMAND != p_interface_desc->bInterfaceProtocol)
  {
    return TUSB_ERROR_CDCH_UNSUPPORTED_PROTOCOL;
  }

  uint8_t const * p_desc = descriptor_next ( (uint8_t const *) p_interface_desc );
  cdch_data_t * p_cdc = &cdch_data[dev_addr-1];

  p_cdc->interface_number   = p_interface_desc->bInterfaceNumber;
  p_cdc->interface_protocol = p_interface_desc->bInterfaceProtocol; // TODO 0xff is consider as rndis candidate, other is virtual Com

  //------------- Communication Interface -------------//
  (*p_length) = sizeof(tusb_descriptor_interface_t);

  while( TUSB_DESC_TYPE_INTERFACE_CLASS_SPECIFIC == p_desc[DESCRIPTOR_OFFSET_TYPE] )
  { // Communication Functional Descriptors
    if ( CDC_FUNC_DESC_ABSTRACT_CONTROL_MANAGEMENT == functional_desc_typeof(p_desc) )
    { // save ACM bmCapabilities
      p_cdc->acm_capability = ((tusb_cdc_func_abstract_control_management_t const *) p_desc)->bmCapabilities;
    }

    (*p_length) += p_desc[DESCRIPTOR_OFFSET_LENGTH];
    p_desc = descriptor_next(p_desc);
  }

  if ( TUSB_DESC_TYPE_ENDPOINT == p_desc[DESCRIPTOR_OFFSET_TYPE])
  { // notification endpoint if any
    p_cdc->pipe_notification = hcd_pipe_open(dev_addr, (tusb_descriptor_endpoint_t const *) p_desc, TUSB_CLASS_CDC);

    (*p_length) += p_desc[DESCRIPTOR_OFFSET_LENGTH];
    p_desc = descriptor_next(p_desc);

    ASSERT(pipehandle_is_valid(p_cdc->pipe_notification), TUSB_ERROR_HCD_OPEN_PIPE_FAILED);
  }

  //------------- Data Interface (if any) -------------//
  if ( (TUSB_DESC_TYPE_INTERFACE == p_desc[DESCRIPTOR_OFFSET_TYPE]) &&
       (TUSB_CLASS_CDC_DATA      == ((tusb_descriptor_interface_t const *) p_desc)->bInterfaceClass) )
  {
    (*p_length) += p_desc[DESCRIPTOR_OFFSET_LENGTH];
    p_desc = descriptor_next(p_desc);

    // data endpoints expected to be in pairs
    for(uint32_t i=0; i<2; i++)
    {
      tusb_descriptor_endpoint_t const *p_endpoint = (tusb_descriptor_endpoint_t const *) p_desc;
      ASSERT_INT(TUSB_DESC_TYPE_ENDPOINT, p_endpoint->bDescriptorType, TUSB_ERROR_CDCH_DESCRIPTOR_CORRUPTED);

      pipe_handle_t * p_pipe_hdl =  ( p_endpoint->bEndpointAddress &  TUSB_DIR_DEV_TO_HOST_MASK ) ?
          &p_cdc->pipe_in : &p_cdc->pipe_out;

      (*p_pipe_hdl) = hcd_pipe_open(dev_addr, p_endpoint, TUSB_CLASS_CDC);
      ASSERT ( pipehandle_is_valid(*p_pipe_hdl), TUSB_ERROR_HCD_OPEN_PIPE_FAILED );

      (*p_length) += p_desc[DESCRIPTOR_OFFSET_LENGTH];
      p_desc = descriptor_next( p_desc );
    }
  }

  return TUSB_ERROR_NONE;
}

void cdch_isr(pipe_handle_t pipe_hdl, tusb_event_t event)
{

}

void cdch_close(uint8_t dev_addr)
{

}

#endif
