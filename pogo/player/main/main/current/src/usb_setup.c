#include <cyg/io/usb/usb.h>
#include <cyg/io/usb/usbs.h>
#include <cyg/io/usb/usbs_pdiusbd12.h>
#include <cyg/io/usb/usbs_mass_storage.h>
#include <cyg/hal/mmgpio.h>

#include <util/eventq/EventQueueAPI.h>
//#include <core/events/SystemEvents.h>
#include <main/main/EventTypes.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_USB, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_USB);

usb_configuration_descriptor usb_configuration = {
    length:             USB_CONFIGURATION_DESCRIPTOR_LENGTH,
    type:               USB_CONFIGURATION_DESCRIPTOR_TYPE,
    total_length_lo:    USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH_LO(1, 2),
    total_length_hi:    USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH_HI(1, 2),
    number_interfaces:  1,
    configuration_id:   1,
    configuration_str:  3,
    attributes:         USB_CONFIGURATION_DESCRIPTOR_ATTR_REQUIRED |
                        USB_CONFIGURATION_DESCRIPTOR_ATTR_SELF_POWERED,
    max_power:          1
};

usb_interface_descriptor usb_interface = {
    length:             USB_INTERFACE_DESCRIPTOR_LENGTH,
    type:               USB_INTERFACE_DESCRIPTOR_TYPE,
    interface_id:       0,
    alternate_setting:  0,
    number_endpoints:   2,
    interface_class:    0x08,
    interface_subclass: 0x06,
    interface_protocol: 0x50,
    interface_str:      4
};

usb_endpoint_descriptor usb_endpoints[] = {
    {
        length:         USB_ENDPOINT_DESCRIPTOR_LENGTH,
        type:           USB_ENDPOINT_DESCRIPTOR_TYPE,
        endpoint:       USB_ENDPOINT_DESCRIPTOR_ENDPOINT_OUT | 2,
        attributes:     USB_ENDPOINT_DESCRIPTOR_ATTR_BULK,
        max_packet_lo:  64,
        max_packet_hi:  0,
        interval:       0
    },
    {
        length:         USB_ENDPOINT_DESCRIPTOR_LENGTH,
        type:           USB_ENDPOINT_DESCRIPTOR_TYPE,
        endpoint:       USB_ENDPOINT_DESCRIPTOR_ENDPOINT_IN | 2,
        attributes:     USB_ENDPOINT_DESCRIPTOR_ATTR_BULK,
        max_packet_lo:  64,
        max_packet_hi:  0,
        interval:       0
    }
};

char language_id_str[] = 
{
    0x04,
    0x03,
    0x09,
    0x04
};

char manufacturer_str[] = 
{
    0x12,  // 0x0c
    0x03,
    'I', 0x00,
    'O', 0x00,
    'b', 0x00,
    'j', 0x00,
    'e', 0x00,
    'c', 0x00,
    't', 0x00,
    's', 0x00
};

char product_str[] = 
{
    0x0e,
    0x03,
    'D', 0x00,
    'h', 0x00,
    'a', 0x00,
    'r', 0x00,
    'm', 0x00,
    'a', 0x00
};

char configuration_str[] = 
{
    0x1a,
    0x03,
    'M', 0x00,
    'a', 0x00,
    's', 0x00,
    's', 0x00,
    ' ', 0x00,
    'S', 0x00,
    't', 0x00,
    'o', 0x00,
    'r', 0x00,
    'a', 0x00,
    'g', 0x00,
    'e', 0x00
};

char interface_str[] = 
{
    0x1a,
    0x03,
    'M', 0x00,
    'a', 0x00,
    's', 0x00,
    's', 0x00,
    ' ', 0x00,
    'S', 0x00,
    't', 0x00,
    'o', 0x00,
    'r', 0x00,
    'a', 0x00,
    'g', 0x00,
    'e', 0x00
};

char serial_number_str[] = 
{
    0x22,
    0x03,
    '0', 0x00,
    '0', 0x00,
    '3', 0x00,
    '2', 0x00,
    '4', 0x00,
    '0', 0x00,
    '4', 0x00,
    '3', 0x00,
    '7', 0x00,
    'B', 0x00,
    '8', 0x00,
    '3', 0x00,
    '0', 0x00,
    '2', 0x00,
    '1', 0x00,
    '8', 0x00
};

unsigned char* usb_strings[] = {
    language_id_str,
    manufacturer_str,
    product_str,
    configuration_str,
    interface_str,
    serial_number_str
};

usbs_enumeration_data usb_enum_data = {
    device: {
        length:                 USB_DEVICE_DESCRIPTOR_LENGTH,
        type:                   USB_DEVICE_DESCRIPTOR_TYPE,
        usb_spec_lo:            USB_DEVICE_DESCRIPTOR_USB11_LO,
        usb_spec_hi:            USB_DEVICE_DESCRIPTOR_USB11_HI,
        device_class:           USB_DEVICE_DESCRIPTOR_CLASS_INTERFACE,
        device_subclass:        USB_DEVICE_DESCRIPTOR_SUBCLASS_INTERFACE,
        device_protocol:        USB_DEVICE_DESCRIPTOR_PROTOCOL_INTERFACE,
        max_packet_size:        16,
#if 1
	vendor_lo:              0x76,
	vendor_hi:              0x0a,
	product_lo:             0x02,
	product_hi:             0x00,
#else
	vendor_lo:              0xff,
	vendor_hi:              0xff,
	product_lo:             0xff,
	product_hi:             0xff,
#endif
        device_lo:              0x00,
        device_hi:              0x01,
        manufacturer_str:       1,
        product_str:            2,
        serial_number_str:      5,
        number_configurations:  1
    },
    configurations:             &usb_configuration,
    total_number_interfaces:    1,
    interfaces:                 &usb_interface,
    total_number_endpoints:     2,
    endpoints:                  usb_endpoints,
    total_number_strings:       6,
    strings:                    usb_strings
};

#define POOL_SIZE (24*1024)
static unsigned long memory_pool[POOL_SIZE];

const char* const dev_name = "/dev/hda/";
usbs_mass_storage_init_data init = 
{
    (void*) memory_pool,
    POOL_SIZE * 4,
    &dev_name,
    1,
};

static void
_state_change(usbs_control_endpoint* ep, void* cb_data, usbs_state_change change, int old_state ) 
{
    static cyg_uint8 usb_mode = 0;
    switch (change)
    {
        case USBS_STATE_CHANGE_DETACHED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_DETACHED\n");
            break;
        case USBS_STATE_CHANGE_ATTACHED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_ATTACHED\n");
            break;
        case USBS_STATE_CHANGE_POWERED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_POWERED\n");
            break;
        case USBS_STATE_CHANGE_RESET:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_RESET\n");
            break;
        case USBS_STATE_CHANGE_ADDRESSED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_ADDRESSED\n");
            break;
        case USBS_STATE_CHANGE_CONFIGURED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_CONFIGURED\n");
            break;
        case USBS_STATE_CHANGE_DECONFIGURED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_DECONFIGURED\n");
            break;
        case USBS_STATE_CHANGE_SUSPENDED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_SUSPENDED\n");
            break;
        case USBS_STATE_CHANGE_RESUMED:
            DEBUGP( DBG_USB, DBGLEV_INFO, "USBS_STATE_CHANGE_RESUMED\n");
            break;
    }
    if( change == USBS_STATE_CHANGE_ADDRESSED ) {
        DEBUGP( DBG_USB, DBGLEV_INFO, "entering usb mode\n");
        put_event(EVENT_USB_CONNECT, 0);
        usb_mode = 1;
    } else if( usb_mode && change == USBS_STATE_CHANGE_SUSPENDED ) {
        DEBUGP( DBG_USB, DBGLEV_INFO, "exiting usb mode\n");
        put_event(EVENT_USB_DISCONNECT, 0);
        usb_mode = 0;
        return ;  // we dont want to hit the driver state change here
    }

    usbs_mass_storage_state_change_handler( ep, cb_data, change, old_state );
}

void InitUSB(void) 
{
    DEBUGP( DBG_USB, DBGLEV_INFO, "USB Init...\n");
    usbs_dharma_pdiusbd12_ep0.enumeration_data = &usb_enum_data;
    usbs_dharma_pdiusbd12_ep0.state_change_fn = &_state_change;

    usbs_mass_storage_init(&init, &usbs_dharma_pdiusbd12_ep0,&usbs_dharma_pdiusbd12_ep4, &usbs_dharma_pdiusbd12_ep5);
    DEBUGP( DBG_USB, DBGLEV_INFO, "USB Start...\n");
    usbs_start( &usbs_dharma_pdiusbd12_ep0 );
    DEBUGP( DBG_USB, DBGLEV_INFO, "USB Ready\n");
}
