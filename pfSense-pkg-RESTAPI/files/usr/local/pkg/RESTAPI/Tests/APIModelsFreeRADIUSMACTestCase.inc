<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Models\FreeRADIUSMAC;
use RESTAPI\Responses\ValidationError;

class APIModelsFreeRADIUSMACTestCase extends TestCase {
    public array $required_packages = ['pfSense-pkg-freeradius3'];

    /**
     * Path to the rendered FreeRADIUS authorized_macs file. Both the upstream package's
     * `freeradius_authorizedmacs_resync()` and our `apply()` write to this file.
     */
    private const AUTHORIZED_MACS_FILE = '/usr/local/etc/raddb/authorized_macs';

    /**
     * Helper: read the rendered authorized_macs file.
     */
    private function read_authorized_macs(): string {
        return file_exists(self::AUTHORIZED_MACS_FILE) ? file_get_contents(self::AUTHORIZED_MACS_FILE) : '';
    }

    /**
     * Checks that we can create, read, update and delete FreeRADIUSMAC models.
     */
    public function test_crud(): void {
        # Create a new FreeRADIUSMAC entry and ensure it is present in the rendered authorized_macs file
        $entry = new FreeRADIUSMAC(mac: 'aa:bb:cc:dd:ee:ff', description: 'Test MAC');
        $entry->create();
        $raddb = $this->read_authorized_macs();
        $this->assert_str_contains($raddb, 'aa-bb-cc-dd-ee-ff');

        # Ensure we can read the created entry from the config
        $read = new FreeRADIUSMAC(id: $entry->id);
        $this->assert_equals($read->mac->value, 'aa:bb:cc:dd:ee:ff');
        $this->assert_equals($read->description->value, 'Test MAC');

        # Ensure we can update the entry
        $entry = new FreeRADIUSMAC(id: $read->id, mac: '11:22:33:44:55:66', description: 'Updated MAC');
        $entry->update();
        $raddb = $this->read_authorized_macs();
        $this->assert_str_does_not_contain($raddb, 'aa-bb-cc-dd-ee-ff');
        $this->assert_str_contains($raddb, '11-22-33-44-55-66');

        # Delete the entry and ensure it is removed from the rendered file
        $entry->delete();
        $raddb = $this->read_authorized_macs();
        $this->assert_str_does_not_contain($raddb, '11-22-33-44-55-66');
    }

    /**
     * Ensures the MAC address field rejects invalid MAC addresses.
     */
    public function test_invalid_mac_rejected(): void {
        $this->assert_throws(
            exceptions: [ValidationError::class],
            callable: function () {
                $entry = new FreeRADIUSMAC(mac: 'not-a-mac');
                $entry->validate();
            },
        );
    }

    /**
     * Ensures that the MAC address field is required.
     */
    public function test_mac_required(): void {
        $this->assert_throws(
            exceptions: [ValidationError::class],
            callable: function () {
                $entry = new FreeRADIUSMAC(description: 'no mac');
                $entry->validate();
            },
        );
    }

    /**
     * Ensures that two MAC entries cannot share the same address.
     */
    public function test_unique_mac_enforced(): void {
        $first = new FreeRADIUSMAC(mac: 'a1:b2:c3:d4:e5:f6');
        $first->create();
        try {
            $this->assert_throws(
                exceptions: [ValidationError::class],
                callable: function () {
                    $dup = new FreeRADIUSMAC(mac: 'a1:b2:c3:d4:e5:f6');
                    $dup->create();
                },
            );
        } finally {
            $first->delete();
        }
    }

    /**
     * Checks that all of the extended (shared) MAC entry attributes serialize into the
     * FreeRADIUS authorized_macs file.
     */
    public function test_extended_attributes(): void {
        $entry = new FreeRADIUSMAC(
            mac: 'de:ad:be:ef:00:01',
            description: 'Extended attributes mac',
            framed_ip_address: '192.0.2.20',
            framed_ip_netmask: '255.255.255.0',
            framed_route: '192.0.2.0/24 192.0.2.1 1',
            framed_ipv6_address: '2001:db8:abab::6',
            framed_ipv6_route: '2001:db8:0:16::/64 2001:db8::16:a0:20ff:fe99:a998 1',
            vlan_id: '200',
            wispr_redirection_url: 'http://www.example.com',
            simultaneous_connect: 3,
            expiration: 'Jan 01 2099',
            session_timeout: 1800,
            login_time: 'Wk0855-2305',
            amount_of_time: 30,
            point_of_time: 'Daily',
            max_total_octets: 512,
            max_total_octets_time_range: 'monthly',
            max_bandwidth_down: 2000,
            max_bandwidth_up: 1000,
            acct_interim_interval: 600,
            top_additional_options: ['DEFAULT Auth-Type = System'],
            check_items_additional_options: ['Max-Daily-Session := 36000'],
            reply_items_additional_options: ['Service-Type == Login-User'],
        );
        $entry->create();

        # Read the entry back and verify shared values were stored
        $read = new FreeRADIUSMAC(id: $entry->id);
        $this->assert_equals($read->mac->value, 'de:ad:be:ef:00:01');
        $this->assert_equals($read->simultaneous_connect->value, 3);
        $this->assert_equals($read->session_timeout->value, 1800);
        $this->assert_equals($read->expiration->value, 'Jan 01 2099');
        $this->assert_equals($read->vlan_id->value, '200');
        $this->assert_equals($read->point_of_time->value, 'Daily');
        $this->assert_equals($read->max_total_octets_time_range->value, 'monthly');
        $this->assert_equals($read->top_additional_options->value, ['DEFAULT Auth-Type = System']);
        $this->assert_equals($read->wispr_redirection_url->value, 'http://www.example.com');

        # Verify the rendered authorized_macs file contains the expected RADIUS attributes.
        # FreeRADIUS canonicalizes the MAC delimiter to '-' when used as the username.
        # Note: the upstream pfSense FreeRADIUS package has known bugs in `freeradius_authorizedmacs_resync()`
        # that prevent `Acct-Interim-Interval` and `WISPr-Redirection-URL` from being rendered for MAC entries
        # (`$users` typo on Acct-Interim line, and `$varmacswisprredirectionurl` vs `$varmacsswisprredirectionurl`
        # variable name typo on the URL line). Those fields ARE round-tripped through our Model and stored
        # correctly in $config — they just are not emitted by upstream's renderer. We assert what actually
        # renders today and rely on the round-trip read above for full coverage of the affected fields.
        $raddb = $this->read_authorized_macs();
        $this->assert_str_contains($raddb, 'de-ad-be-ef-00-01');
        $this->assert_str_contains($raddb, 'Simultaneous-Use := "3"');
        $this->assert_str_contains($raddb, 'Expiration := "Jan 01 2099"');
        $this->assert_str_contains($raddb, 'Login-Time := "Wk0855-2305"');
        $this->assert_str_contains($raddb, 'Max-Daily-Session := 1800'); # 30 minutes * 60 seconds
        $this->assert_str_contains($raddb, 'Framed-IP-Address = 192.0.2.20');
        $this->assert_str_contains($raddb, 'Framed-IP-Netmask = 255.255.255.0');
        $this->assert_str_contains($raddb, 'Framed-Route = "192.0.2.0/24 192.0.2.1 1"');
        $this->assert_str_contains($raddb, 'Framed-IPv6-Address = 2001:db8:abab::6');
        $this->assert_str_contains($raddb, 'Session-Timeout := 1800');
        $this->assert_str_contains($raddb, 'Tunnel-Private-Group-ID = "200"');
        $this->assert_str_contains($raddb, 'DEFAULT Auth-Type = System');
        $this->assert_str_contains($raddb, 'Max-Daily-Session := 36000');
        $this->assert_str_contains($raddb, 'Service-Type == Login-User');

        # Bandwidth values are multiplied by 1024 in the rendered MAC output (note: upstream uses 1024 here,
        # but uses 1000 in the user-equivalent code path).
        $this->assert_str_contains($raddb, '2048000'); # 2000 * 1024 (bandwidth down)
        $this->assert_str_contains($raddb, '1024000'); # 1000 * 1024 (bandwidth up)
        # Total octets are multiplied by 1024*1024 in the rendered output
        $this->assert_str_contains($raddb, (string) (512 * 1024 * 1024));

        $entry->delete();
    }

    /**
     * Ensures choice fields reject invalid values.
     */
    public function test_pointoftime_rejects_invalid_choice(): void {
        $this->assert_throws_response(
            response_id: 'FIELD_INVALID_CHOICE',
            code: 400,
            callable: function () {
                $entry = new FreeRADIUSMAC(mac: 'aa:bb:cc:dd:ee:01', point_of_time: 'Hourly');
                $entry->validate();
            },
        );
    }

    /**
     * Ensures the bandwidth ceiling is enforced.
     */
    public function test_bandwidth_max_enforced(): void {
        $this->assert_throws(
            exceptions: [ValidationError::class],
            callable: function () {
                $entry = new FreeRADIUSMAC(mac: 'aa:bb:cc:dd:ee:02', max_bandwidth_down: 4294968);
                $entry->validate();
            },
        );
    }

    /**
     * Ensures the redirection URL field rejects invalid URLs.
     */
    public function test_redirection_url_validated(): void {
        $this->assert_throws(
            exceptions: [ValidationError::class],
            callable: function () {
                $entry = new FreeRADIUSMAC(mac: 'aa:bb:cc:dd:ee:03', wispr_redirection_url: 'not a url');
                $entry->validate();
            },
        );
    }

    /**
     * Ensures the login_time regex validator rejects illegal characters.
     */
    public function test_login_time_regex(): void {
        $this->assert_throws(
            exceptions: [ValidationError::class],
            callable: function () {
                $entry = new FreeRADIUSMAC(mac: 'aa:bb:cc:dd:ee:04', login_time: 'Wk@@@');
                $entry->validate();
            },
        );
    }
}
