// SPDX-License-Identifier: GPL-2.0+

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/module.h>

#include "hid-ids.h"

#define FIX_SHARE_BUTTON 0x01

struct microsoft_xbox_sc {
	unsigned long quirks;
};


#define microsoft_xbox_map_axis_usage_clear(c) hid_map_usage_clear(hi, usage, bit, max, EV_ABS, (c))
static int microsoft_xbox_input_mapping(struct hid_device *hdev, struct hid_input *hi,
				 struct hid_field *field,
				 struct hid_usage *usage, unsigned long **bit, int *max)
{
        switch (usage->hid) {
	case HID_GD_Z:
		hid_map_usage_clear(hi, usage, bit, max, EV_ABS, ABS_RX);
		break;
	case HID_GD_RZ:
		hid_map_usage_clear(hi, usage, bit, max, EV_ABS, ABS_RY);
		break;
	case (HID_UP_SIMULATION | 0x00c4): // gas
		hid_map_usage_clear(hi, usage, bit, max, EV_ABS, ABS_RZ);
		break;
	case (HID_UP_SIMULATION | 0x00c5): // break
		hid_map_usage_clear(hi, usage, bit, max, EV_ABS, ABS_Z);
		break;
	default:
		return 0;
	}

	/* let HID handle this */
	return 1;
}

static int microsoft_xbox_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	unsigned long quirks = id->driver_data;
	struct microsoft_xbox_sc *xsc;
	int ret;

	xsc = devm_kzalloc(&hdev->dev, sizeof(*xsc), GFP_KERNEL);
	if (xsc == NULL) {
		hid_err(hdev, "can't alloc microsoft_xbox descriptor\n");
		return -ENOMEM;
	}

	xsc->quirks = quirks;
	hid_set_drvdata(hdev, xsc);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		return ret;
	}

	return 0;
}

static const struct hid_device_id microsoft_xbox_devices[] = {
	/* XBOX ONE S / X */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02FD) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02E0) },
	/* XBOX ONE Elite Series 2 */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B05) },
	/* XBOX Series X|S */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B13),
		.driver_data = FIX_SHARE_BUTTON },
	{ }
};
MODULE_DEVICE_TABLE(hid, microsoft_xbox_devices);

static struct hid_driver microsoft_xbox_driver = {
	.name = "microsoft_xbox",
	.id_table = microsoft_xbox_devices,
	.input_mapping = microsoft_xbox_input_mapping,
	.probe = microsoft_xbox_probe,
};
module_hid_driver(microsoft_xbox_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_AUTHOR("Kai Krakow <kai@kaishome.de>");
MODULE_AUTHOR("Jelle van der Waa <jvanderwaa@redhat.com>");
