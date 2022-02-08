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

struct usage_map {
	u32 usage;
	enum {
		MAP_IGNORE = -1,	/* Completely ignore this field */
		MAP_AUTO,		/* Do not really map it, let hid-core decide */
		MAP_STATIC		/* Map to the values given */
	} behaviour;
	struct {
		u8 event_type;	/* input event (EV_KEY, EV_ABS, ...) */
		u16 input_code;	/* input code (BTN_A, ABS_X, ...) */
	} ev;
};

#define USAGE_MAP(u, b, e, i) \
	{ .usage = (u), .behaviour = (b), .ev = { .event_type = (e), .input_code = (i) } }
#define USAGE_IGN(u) USAGE_MAP(u, MAP_IGNORE, 0, 0)


static const struct usage_map microsoft_xbox_usage_maps[] = {
	/* fixup buttons to Linux codes */
	USAGE_MAP(0x90001, MAP_STATIC, EV_KEY, BTN_A),	/* A */
	USAGE_MAP(0x90002, MAP_STATIC, EV_KEY, BTN_B),	/* B */
	USAGE_MAP(0x90003, MAP_STATIC, EV_KEY, BTN_X),	/* X */
	USAGE_MAP(0x90004, MAP_STATIC, EV_KEY, BTN_Y),	/* Y */
	USAGE_MAP(0x90005, MAP_STATIC, EV_KEY, BTN_TL),	/* LB */
	USAGE_MAP(0x90006, MAP_STATIC, EV_KEY, BTN_TR),	/* RB */
	USAGE_MAP(0x90007, MAP_STATIC, EV_KEY, BTN_SELECT),	/* Back */
	USAGE_MAP(0x90008, MAP_STATIC, EV_KEY, BTN_START),	/* Menu */
	USAGE_MAP(0x90009, MAP_STATIC, EV_KEY, BTN_THUMBL),	/* LS */
	USAGE_MAP(0x9000A, MAP_STATIC, EV_KEY, BTN_THUMBR),	/* RS */

	/* fixup the Xbox logo button */
	USAGE_MAP(0x9000B, MAP_STATIC, EV_KEY, KEY_MODE),	/* Xbox */

	/* fixup the Share button */
	USAGE_MAP(0x9000C, MAP_STATIC, EV_KEY, KEY_RECORD),	/* Share */

	/* fixup code "Sys Main Menu" from Windows report descriptor */
	USAGE_MAP(0x10085, MAP_STATIC, EV_KEY, KEY_MODE),

	/* fixup code "AC Home" from Linux report descriptor */
	USAGE_MAP(0xC0223, MAP_STATIC, EV_KEY, KEY_MODE),

	/* fixup code "AC Back" from Linux report descriptor */
	USAGE_MAP(0xC0224, MAP_STATIC, EV_KEY, BTN_SELECT),

	/* hardware features handled at the raw report level */
	USAGE_IGN(0xC0085),	/* Profile switcher */
	USAGE_IGN(0xC0099),	/* Trigger scale switches */

	/* XBE2: Disable "dial", which is a redundant representation of the D-Pad */
	USAGE_IGN(0x10037),

	/* XBE2: Disable duplicate report fields of broken v1 packet format */
	USAGE_IGN(0x10040),	/* Vx, copy of X axis */
	USAGE_IGN(0x10041),	/* Vy, copy of Y axis */
	USAGE_IGN(0x10042),	/* Vz, copy of Z axis */
	USAGE_IGN(0x10043),	/* Vbrx, copy of Rx */
	USAGE_IGN(0x10044),	/* Vbry, copy of Ry */
	USAGE_IGN(0x10045),	/* Vbrz, copy of Rz */
	USAGE_IGN(0x90010),	/* copy of A */
	USAGE_IGN(0x90011),	/* copy of B */
	USAGE_IGN(0x90013),	/* copy of X */
	USAGE_IGN(0x90014),	/* copy of Y */
	USAGE_IGN(0x90016),	/* copy of LB */
	USAGE_IGN(0x90017),	/* copy of RB */
	USAGE_IGN(0x9001B),	/* copy of Start */
	USAGE_IGN(0x9001D),	/* copy of LS */
	USAGE_IGN(0x9001E),	/* copy of RS */
	USAGE_IGN(0xC0082),	/* copy of Select button */

	/* XBE2: Disable extra features until proper support is implemented */
	USAGE_IGN(0xC0081),	/* Four paddles */

	/* XBE2: Disable unused buttons */
	USAGE_IGN(0x90012),	/* 6 "TRIGGER_HAPPY" buttons */
	USAGE_IGN(0x90015),
	USAGE_IGN(0x90018),
	USAGE_IGN(0x90019),
	USAGE_IGN(0x9001A),
	USAGE_IGN(0x9001C),
	USAGE_IGN(0xC00B9),	/* KEY_SHUFFLE button */
};

#define microsoft_xbox_map_usage_clear(ev) hid_map_usage_clear(hi, usage, bit, max, (ev).event_type, (ev).input_code)
static int microsoft_xbox_input_mapping(struct hid_device *hdev, struct hid_input *hi,
				 struct hid_field *field,
				 struct hid_usage *usage, unsigned long **bit, int *max)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(microsoft_xbox_usage_maps); i++) {
		const struct usage_map *entry = &microsoft_xbox_usage_maps[i];

		if (entry->usage == usage->hid) {
			if (entry->behaviour == MAP_STATIC)
				microsoft_xbox_map_usage_clear(entry->ev);
			return entry->behaviour;
		}
	}

	/* let HID handle this */
	return MAP_AUTO;
}

static u8 *microsoft_xbox_report_fixup(struct hid_device *hdev, u8 *rdesc, unsigned int *rsize)
{
	hid_info(hdev, "report descriptor size: %d bytes\n", *rsize);

	/* fixup reported axes for Xbox One S and Xbox Series X|S */
	if (*rsize >= 81) {
		if (rdesc[34] == 0x09 && rdesc[35] == 0x32) {
			hid_notice(hdev, "fixing up Rx axis\n");
			rdesc[35] = 0x33;	/* Z --> Rx */
		}
		if (rdesc[36] == 0x09 && rdesc[37] == 0x35) {
			hid_notice(hdev, "fixing up Ry axis\n");
			rdesc[37] = 0x34;	/* Rz --> Ry */
		}
		if (rdesc[52] == 0x05 && rdesc[53] == 0x02 &&
		    rdesc[54] == 0x09 && rdesc[55] == 0xC5) {
			hid_notice(hdev, "fixing up Z axis\n");
			rdesc[53] = 0x01;	/* Simulation -> Gendesk */
			rdesc[55] = 0x32;	/* Brake -> Z */
		}
		if (rdesc[77] == 0x05 && rdesc[78] == 0x02 &&
		    rdesc[79] == 0x09 && rdesc[80] == 0xC4) {
			hid_notice(hdev, "fixing up Rz axis\n");
			rdesc[78] = 0x01;	/* Simulation -> Gendesk */
			rdesc[80] = 0x35;	/* Accelerator -> Rz */
		}
	}

	/* fixup reported button count for Xbox controllers in Linux mode */
	if (*rsize >= 164) {
		/*
		 * 12 buttons instead of 10: properly remap the
		 * Xbox button (button 11)
		 * Share button (button 12)
		 */
		if (rdesc[140] == 0x05 && rdesc[141] == 0x09 &&
		    rdesc[144] == 0x29 && rdesc[145] == 0x0F &&
		    rdesc[152] == 0x95 && rdesc[153] == 0x0F &&
		    rdesc[162] == 0x95 && rdesc[163] == 0x01) {
			hid_notice(hdev, "fixing up button mapping\n");
			//xdata->quirks |= XPADNEO_QUIRK_LINUX_BUTTONS;
			rdesc[145] = 0x0C;	/* 15 buttons -> 12 buttons */
			rdesc[153] = 0x0C;	/* 15 bits -> 12 bits buttons */
			rdesc[163] = 0x04;	/* 1 bit -> 4 bits constants */
		}
	}

	return rdesc;
}

static int microsoft_xbox_raw_event(struct hid_device *hdev, struct hid_report *report,
			     u8 *data, int reportsize)
{
	struct microsoft_xbox_sc *xsc = hid_get_drvdata(hdev);
	/* correct button mapping of Xbox controllers in Linux mode */
	if (report->id == 1 && reportsize >= 17) {
		u16 bits = 0;

		bits |= (data[14] & (BIT(0) | BIT(1))) >> 0;	/* A, B */
		bits |= (data[14] & (BIT(3) | BIT(4))) >> 1;	/* X, Y */
		bits |= (data[14] & (BIT(6) | BIT(7))) >> 2;	/* LB, RB */

		if (xsc->quirks & FIX_SHARE_BUTTON)
			bits |= (data[15] & BIT(2)) << 4;	/* Back */
		else
			bits |= (data[16] & BIT(0)) << 6;	/* Back */

		bits |= (data[15] & BIT(3)) << 4;	/* Menu */
		bits |= (data[15] & BIT(5)) << 3;	/* LS */
		bits |= (data[15] & BIT(6)) << 3;	/* RS */
		bits |= (data[15] & BIT(4)) << 6;	/* Xbox */

		if (xsc->quirks & FIX_SHARE_BUTTON)
			bits |= (data[16] & BIT(0)) << 11;	/* Share */

		data[14] = (u8)((bits >> 0) & 0xFF);
		data[15] = (u8)((bits >> 8) & 0xFF);
		data[16] = 0;
	}

	return 0;
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
	.report_fixup = microsoft_xbox_report_fixup,
	.raw_event = microsoft_xbox_raw_event,
	.probe = microsoft_xbox_probe,
};
module_hid_driver(microsoft_xbox_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_AUTHOR("Kai Krakow <kai@kaishome.de>");
MODULE_AUTHOR("Jelle van der Waa <jvanderwaa@redhat.com>");