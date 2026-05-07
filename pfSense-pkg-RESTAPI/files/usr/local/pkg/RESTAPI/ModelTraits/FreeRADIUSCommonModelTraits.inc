<?php

namespace RESTAPI\ModelTraits;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Fields\IntegerField;
use RESTAPI\Fields\StringField;
use RESTAPI\Validators\IPAddressValidator;
use RESTAPI\Validators\RegexValidator;
use RESTAPI\Validators\URLValidator;

/**
 * Defines a set of Field declarations and helpers that are shared between the FreeRADIUS user-style Models
 * (FreeRADIUSUser, FreeRADIUSMAC). Both pfSense FreeRADIUS user and authorized-MAC entries expose the same
 * set of RADIUS attribute fields (description, framed addressing, VLAN, time/quota, bandwidth, additional
 * RADIUS attributes, etc.) — the only difference is the internal pfSense config key prefix
 * (`varusers...` vs `varmacs...`) and a small naming quirk on the WISPr redirection URL field.
 */
trait FreeRADIUSCommonModelTraits {
    public StringField $description;
    public StringField $framed_ip_address;
    public StringField $framed_ip_netmask;
    public StringField $framed_route;
    public StringField $framed_ipv6_address;
    public StringField $framed_ipv6_route;
    public StringField $vlan_id;
    public StringField $wispr_redirection_url;
    public IntegerField $simultaneous_connect;
    public StringField $expiration;
    public IntegerField $session_timeout;
    public StringField $login_time;
    public IntegerField $amount_of_time;
    public StringField $point_of_time;
    public IntegerField $max_total_octets;
    public StringField $max_total_octets_time_range;
    public IntegerField $max_bandwidth_down;
    public IntegerField $max_bandwidth_up;
    public IntegerField $acct_interim_interval;
    public StringField $top_additional_options;
    public StringField $check_items_additional_options;
    public StringField $reply_items_additional_options;

    /**
     * Defines the set of Fields that are shared between the FreeRADIUSUser and FreeRADIUSMAC models.
     *
     * @param string $prefix The internal pfSense config key prefix used by the consuming Model
     *      (e.g. `varusers` for users or `varmacs` for MAC entries).
     * @param string $url_field_suffix The suffix appended to $prefix to compute the internal_name
     *      for the WISPr redirection URL field. The pfSense package uses `wisprredirectionurl` for
     *      users and the (typo'd) `swisprredirectionurl` for MACs, so this is overridable.
     */
    protected function init_freeradius_common_fields(
        string $prefix,
        string $url_field_suffix = 'wisprredirectionurl',
    ): void {
        $this->description = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'Description',
            validators: [
                new RegexValidator(
                    pattern: '/^[a-zA-Z0-9 _,.;:+=()-]*$/',
                    error_msg: 'Value contains invalid characters.',
                ),
            ],
            help_text: 'A description for this entry.',
        );
        $this->framed_ip_address = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'Framed IP Address',
            internal_name: "{$prefix}framedipaddress",
            validators: [new IPAddressValidator(allow_ipv4: true, allow_ipv6: false)],
            help_text: 'Framed-IP-Address MUST be supported by NAS. ' .
                'If the OpenVPN server uses a subnet style Topology the RADIUS server MUST ' .
                'also send back an appropriate Framed-IP-Netmask value matching the VPN Tunnel Network.',
        );
        $this->framed_ip_netmask = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'Framed IP Netmask',
            internal_name: "{$prefix}framedipnetmask",
            validators: [new IPAddressValidator(allow_ipv4: true, allow_ipv6: false)],
            help_text: 'Framed-IP-Netmask MUST be supported by NAS.',
        );
        $this->framed_route = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'IPv4 Gateway',
            internal_name: "{$prefix}framedroute",
            help_text: 'Framed-Route must be supported by NAS. Required format: ' .
                'Subnet Gateway Metric(s) (e.g. 192.168.10.0/24 192.168.10.1 1).',
        );
        $this->framed_ipv6_address = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'IPv6 Address',
            internal_name: "{$prefix}framedip6address",
            help_text: 'When the IPv6 prefix part is empty it uses Framed-IPv6-Address. ' .
                'When the prefix part is filled in, it uses Framed-IPv6-Prefix. ' .
                'Example: 2001:db8:abab::5 or 2001:db8:abab::/64',
        );
        $this->framed_ipv6_route = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'IPv6 Gateway',
            internal_name: "{$prefix}framedip6route",
            help_text: 'Framed-IPv6-Route must be supported by NAS. Required format: ' .
                'Prefix Gateway Metric(s) (e.g. 2001:db8:0:16::/64 2001:db8::16:a0:20ff:fe99:a998 1).',
        );
        $this->vlan_id = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'VLAN ID',
            internal_name: "{$prefix}vlanid",
            help_text: 'The VLAN ID (integer from 1-4095) or the VLAN name that this entry should be assigned to. ' .
                'Must be supported by the NAS.',
        );
        $this->wispr_redirection_url = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'Redirection URL',
            internal_name: "{$prefix}{$url_field_suffix}",
            validators: [new URLValidator()],
            help_text: 'The URL the user should be redirected to after successful login. ' .
                'Example: http://www.google.com',
        );
        $this->simultaneous_connect = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 0,
            verbose_name: 'Simultaneous Connections',
            internal_name: "{$prefix}simultaneousconnect",
            help_text: 'The maximum number of simultaneous connections with this entry. Leave null for no limit. ' .
                'If using FreeRADIUS with Captive Portal you should leave this null.',
        );
        $this->expiration = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'Expiration Date',
            internal_name: "{$prefix}expiration",
            help_text: 'The date when this account should expire. Required format: Mmm dd yyyy (e.g. Jan 01 2030).',
        );
        $this->session_timeout = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 0,
            verbose_name: 'Session Timeout',
            internal_name: "{$prefix}sessiontimeout",
            help_text: 'The time this entry has until relogin (in seconds).',
        );
        $this->login_time = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            verbose_name: 'Possible Login Times',
            internal_name: "{$prefix}logintime",
            validators: [
                new RegexValidator(
                    pattern: '/^[a-zA-Z0-9,|-]*$/',
                    error_msg: 'Value may only contain a-z, A-Z, 0-9, comma, vertical bar and hyphen.',
                ),
            ],
            help_text: 'The time when this entry should have access. Empty for no time restriction. ' .
                'Example: Wk0855-2305,Sa|Su2230-0230 ' .
                '(weekdays after 8:55 AM and before 11:05 PM | any time on Saturday | ' .
                'Sunday after 10:30 PM and before 02:30 AM).',
        );
        $this->amount_of_time = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 0,
            verbose_name: 'Amount of Time',
            internal_name: "{$prefix}amountoftime",
            help_text: 'The amount of time this entry is allowed (in minutes) within the configured time period.',
        );
        $this->point_of_time = new StringField(
            required: false,
            default: 'Daily',
            choices: ['Daily', 'Weekly', 'Monthly', 'Forever'],
            verbose_name: 'Time Period',
            internal_name: "{$prefix}pointoftime",
            help_text: "The time period after which the 'Amount of Time' is reset.",
        );
        $this->max_total_octets = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 0,
            verbose_name: 'Max Total Octets',
            internal_name: "{$prefix}maxtotaloctets",
            help_text: 'The amount of download and upload traffic (summarized) in megabytes (MB) for this entry. ' .
                'If using captive portal without periodic reauthentication enabled, this value must not exceed ' .
                '4095 due to protocol limitations.',
        );
        $this->max_total_octets_time_range = new StringField(
            required: false,
            default: 'daily',
            choices: ['daily', 'weekly', 'monthly', 'forever'],
            verbose_name: 'Max Total Octets Time Range',
            internal_name: "{$prefix}maxtotaloctetstimerange",
            help_text: 'The time period for the amount of download and upload traffic. ' .
                'This does not automatically reset the counter; you must configure a cronjob to reset it.',
        );
        $this->max_bandwidth_down = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 0,
            maximum: 4294967,
            verbose_name: 'Max Bandwidth Down',
            internal_name: "{$prefix}maxbandwidthdown",
            help_text: 'The maximum bandwidth for download in kilobits (1000 bits) per second (Kbit/s).',
        );
        $this->max_bandwidth_up = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 0,
            maximum: 4294967,
            verbose_name: 'Max Bandwidth Up',
            internal_name: "{$prefix}maxbandwidthup",
            help_text: 'The maximum bandwidth for upload in kilobits (1000 bits) per second (Kbit/s).',
        );
        $this->acct_interim_interval = new IntegerField(
            required: false,
            default: null,
            allow_null: true,
            minimum: 61,
            verbose_name: 'Accounting Interim Interval',
            internal_name: "{$prefix}acctinteriminterval",
            help_text: 'The interval in seconds which should elapse between interim-updates. ' .
                'Must be more than 60s and should not be less than 600s.',
        );
        $this->top_additional_options = new StringField(
            required: false,
            default: [],
            allow_empty: true,
            many: true,
            delimiter: '|',
            verbose_name: 'Additional Top Options',
            internal_name: "{$prefix}topadditionaloptions",
            help_text: 'Additional RADIUS attributes placed at the TOP of this entry. ' .
                'Each list entry becomes a separate line. For experts only.',
        );
        $this->check_items_additional_options = new StringField(
            required: false,
            default: [],
            allow_empty: true,
            many: true,
            delimiter: '|',
            verbose_name: 'Additional Check-Item Options',
            internal_name: "{$prefix}checkitemsadditionaloptions",
            help_text: 'Additional RADIUS check-item attributes for this entry. ' .
                'Each list entry becomes a separate attribute. For experts only.',
        );
        $this->reply_items_additional_options = new StringField(
            required: false,
            default: [],
            allow_empty: true,
            many: true,
            delimiter: '|',
            verbose_name: 'Additional Reply-Item Options',
            internal_name: "{$prefix}replyitemsadditionaloptions",
            help_text: 'Additional RADIUS reply-item attributes for this entry. ' .
                'Each list entry becomes a separate attribute. For experts only.',
        );
    }
}
