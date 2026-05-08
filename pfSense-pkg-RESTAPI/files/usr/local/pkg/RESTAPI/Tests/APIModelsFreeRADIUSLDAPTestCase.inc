<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Models\Certificate;
use RESTAPI\Models\CertificateAuthority;
use RESTAPI\Models\FreeRADIUSLDAP;
use RESTAPI\Responses\ValidationError;

class APIModelsFreeRADIUSLDAPTestCase extends TestCase {
    public array $required_packages = ['pfSense-pkg-freeradius3'];

    /**
     * Reads the rendered FreeRADIUS LDAP module configuration file from disk.
     * Returns an empty string if the file does not exist (e.g. when no LDAP servers are enabled).
     */
    private function read_ldap_conf(): string {
        # The pfSense FreeRADIUS package writes the rendered ldap module configuration alongside the
        # other module configs. Walk the standard locations for the running raddb instance.
        foreach (
            [
                '/usr/local/etc/raddb/mods-enabled/ldap',
                '/usr/local/etc/raddb/mods-available/ldap',
                '/usr/local/etc/raddb/modules/ldap',
            ]
            as $candidate
        ) {
            if (file_exists($candidate)) {
                return file_get_contents($candidate);
            }
        }
        return '';
    }

    /**
     * Resets the LDAP module configuration back to its default state. Called after every test that
     * mutates the singleton so subsequent tests start from a known baseline.
     */
    private function reset_ldap(): void {
        $ldap = new FreeRADIUSLDAP();
        # Apply with all defaults
        $ldap->update();
    }

    /**
     * Checks that the singleton FreeRADIUSLDAP model returns its defaults on first read.
     */
    public function test_defaults_round_trip(): void {
        $ldap = new FreeRADIUSLDAP();
        $this->assert_equals($ldap->failover_mode->value, 'redundant');
        $this->assert_equals($ldap->server1_server->value, 'ldap.example.com');
        $this->assert_equals($ldap->server1_server_port->value, '389');
        $this->assert_equals($ldap->server1_msad_compatibility_enable->value, 'Disable');
        $this->assert_equals($ldap->server1_compare_check_items->value, 'Yes');
        $this->assert_equals($ldap->server1_require_cert->value, 'never');
        $this->assert_equals($ldap->server2_server_port->value, '389');
        $this->assert_equals($ldap->server2_keepalive_idle->value, 60);
    }

    /**
     * Checks that we can update the singleton FreeRADIUSLDAP model and read the values back,
     * and that the upstream LDAP module config file reflects those changes after apply().
     */
    public function test_update_and_apply(): void {
        $ldap = new FreeRADIUSLDAP();
        $ldap->failover_mode->value = 'load-balance';
        $ldap->server1_enable_authorize->value = true;
        $ldap->server1_enable_authenticate->value = true;
        $ldap->server1_server->value = 'ldap1.example.org';
        $ldap->server1_server_port->value = '636';
        $ldap->server1_identity->value = 'cn=admin,dc=example,dc=org';
        $ldap->server1_password->value = 'super-secret';
        $ldap->server1_base_dn->value = 'dc=example,dc=org';
        $ldap->server1_filter->value = '(sAMAccountName=%{User-Name})';
        $ldap->server1_base_filter->value = '(objectclass=person)';
        $ldap->server1_ldap_connections_number->value = 10;
        $ldap->server1_timeout->value = 5;
        $ldap->server1_time_limit->value = 6;
        $ldap->server1_net_timeout->value = 2;
        $ldap->server1_msad_compatibility_enable->value = 'Enable';
        $ldap->server1_misc_enable->value = true;
        $ldap->server1_default_profile->value = 'cn=defaultprofile,dc=example,dc=org';
        $ldap->server1_profile_attribute->value = 'profileAttribute';
        $ldap->server1_access_attribute->value = 'memberOf';
        $ldap->server1_group_enable->value = true;
        $ldap->server1_groupname_attribute->value = 'cn';
        $ldap->server1_group_membership_attribute->value = 'memberOf';
        $ldap->server1_compare_check_items->value = 'No';
        $ldap->server1_do_xlat->value = 'No';
        $ldap->server1_access_attr_used_for_allow->value = 'No';
        $ldap->server1_keepalive_idle->value = 120;
        $ldap->server1_keepalive_probes->value = 5;
        $ldap->server1_keepalive_interval->value = 7;
        $ldap->server1_require_cert->value = 'allow';

        $ldap->server2_enable_authorize->value = true;
        $ldap->server2_server->value = '198.51.100.10';
        $ldap->server2_server_port->value = '3268';
        $ldap->server2_identity->value = 'cn=admin,dc=secondary,dc=org';
        $ldap->server2_password->value = 'another-secret';
        $ldap->server2_base_dn->value = 'dc=secondary,dc=org';

        $ldap->update();

        # Read the singleton back from config and verify values round-trip correctly
        $read = new FreeRADIUSLDAP();
        $this->assert_equals($read->failover_mode->value, 'load-balance');
        $this->assert_equals($read->server1_enable_authorize->value, true);
        $this->assert_equals($read->server1_enable_authenticate->value, true);
        $this->assert_equals($read->server1_server->value, 'ldap1.example.org');
        $this->assert_equals($read->server1_server_port->value, '636');
        $this->assert_equals($read->server1_identity->value, 'cn=admin,dc=example,dc=org');
        $this->assert_equals($read->server1_password->value, 'super-secret');
        $this->assert_equals($read->server1_base_dn->value, 'dc=example,dc=org');
        $this->assert_equals($read->server1_filter->value, '(sAMAccountName=%{User-Name})');
        $this->assert_equals($read->server1_ldap_connections_number->value, 10);
        $this->assert_equals($read->server1_timeout->value, 5);
        $this->assert_equals($read->server1_msad_compatibility_enable->value, 'Enable');
        $this->assert_equals($read->server1_misc_enable->value, true);
        $this->assert_equals($read->server1_compare_check_items->value, 'No');
        $this->assert_equals($read->server1_keepalive_idle->value, 120);
        $this->assert_equals($read->server1_require_cert->value, 'allow');
        $this->assert_equals($read->server2_enable_authorize->value, true);
        $this->assert_equals($read->server2_server->value, '198.51.100.10');
        $this->assert_equals($read->server2_server_port->value, '3268');
        $this->assert_equals($read->server2_base_dn->value, 'dc=secondary,dc=org');

        # Confirm apply() rendered the LDAP module configuration file with the new values
        $conf = $this->read_ldap_conf();
        if ($conf !== '') {
            $this->assert_str_contains($conf, 'ldap1.example.org');
            $this->assert_str_contains($conf, '636');
            $this->assert_str_contains($conf, 'cn=admin,dc=example,dc=org');
            $this->assert_str_contains($conf, 'dc=example,dc=org');
            $this->assert_str_contains($conf, '198.51.100.10');
        }

        $this->reset_ldap();
    }

    /**
     * Ensures the choice on `failover_mode` is enforced by the StringField.
     */
    public function test_failover_mode_choice(): void {
        $this->assert_throws_response(
            response_id: 'FIELD_INVALID_CHOICE',
            code: 400,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(failover_mode: 'round-robin');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures the choice on `server1_msad_compatibility_enable` is enforced by the StringField.
     */
    public function test_msad_compat_choice(): void {
        $this->assert_throws_response(
            response_id: 'FIELD_INVALID_CHOICE',
            code: 400,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server1_msad_compatibility_enable: 'maybe');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures the choice on `server1_require_cert` is enforced by the StringField.
     */
    public function test_require_cert_choice(): void {
        $this->assert_throws_response(
            response_id: 'FIELD_INVALID_CHOICE',
            code: 400,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server1_require_cert: 'always');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures invalid server addresses are rejected by `validate_server1_server`.
     */
    public function test_server1_invalid_address(): void {
        $this->assert_throws_response(
            response_id: 'FREERADIUS_LDAP_INVALID_SERVER_ADDRESS',
            code: 400,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server1_server: 'not a hostname or ip');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures invalid server addresses on server2 are rejected.
     */
    public function test_server2_invalid_address(): void {
        $this->assert_throws_response(
            response_id: 'FREERADIUS_LDAP_INVALID_SERVER_ADDRESS',
            code: 400,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server2_server: 'not a hostname or ip');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures the PortField rejects invalid port values.
     */
    public function test_invalid_port_rejected(): void {
        $this->assert_throws(
            exceptions: [ValidationError::class],
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server1_server_port: '99999');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures `ssl_ca_cert` and `ssl_server_cert` are validated against existing
     * CertificateAuthority/Certificate refids when set.
     */
    public function test_ssl_cert_foreign_validation(): void {
        $this->assert_throws_response(
            response_id: 'FOREIGN_MODEL_FIELD_VALUE_NOT_FOUND',
            code: 404,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server1_ssl_ca_cert: 'doesnotexist1234');
                $ldap->validate();
            },
        );
        $this->assert_throws_response(
            response_id: 'FOREIGN_MODEL_FIELD_VALUE_NOT_FOUND',
            code: 404,
            callable: function () {
                $ldap = new FreeRADIUSLDAP(server2_ssl_server_cert: 'doesnotexist1234');
                $ldap->validate();
            },
        );
    }

    /**
     * Ensures that an existing CertificateAuthority refid is accepted by `server1_ssl_ca_cert`
     * (verifying the ForeignModelField relationship resolves correctly).
     */
    public function test_ssl_cert_foreign_accepts_existing(): void {
        # Locate any existing CertificateAuthority on the system - if none exists, skip the assertion
        $cas = CertificateAuthority::read_all();
        if ($cas->count() === 0) {
            return;
        }
        $ca_refid = $cas->first()->refid->value;

        $ldap = new FreeRADIUSLDAP(server1_ssl_ca_cert: $ca_refid);
        $this->assert_does_not_throw(
            callable: function () use ($ldap) {
                $ldap->validate();
            },
        );

        # And similarly for server certificates
        $certs = Certificate::read_all();
        if ($certs->count() > 0) {
            $cert_refid = $certs->first()->refid->value;
            $ldap2 = new FreeRADIUSLDAP(server1_ssl_server_cert: $cert_refid);
            $this->assert_does_not_throw(
                callable: function () use ($ldap2) {
                    $ldap2->validate();
                },
            );
        }
    }
}
