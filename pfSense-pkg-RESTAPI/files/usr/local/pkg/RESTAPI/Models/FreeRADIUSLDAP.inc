<?php

namespace RESTAPI\Models;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Model;
use RESTAPI\Fields\BooleanField;
use RESTAPI\Fields\ForeignModelField;
use RESTAPI\Fields\IntegerField;
use RESTAPI\Fields\PortField;
use RESTAPI\Fields\StringField;
use RESTAPI\Responses\ValidationError;

/**
 * Defines a Model that represents the FreeRADIUS LDAP module configuration.
 *
 * The pfSense FreeRADIUS package stores a single LDAP module configuration object that defines
 * up to two LDAP servers (server1 and server2) along with a redundant/load-balance failover mode
 * between them. This Model exposes that configuration as a singleton.
 */
class FreeRADIUSLDAP extends Model {
    # Failover/load balancing selector that pairs the two LDAP servers
    public StringField $failover_mode;

    # Server 1
    public BooleanField $server1_enable_authorize;
    public BooleanField $server1_enable_authenticate;
    public StringField $server1_server;
    public PortField $server1_server_port;
    public StringField $server1_identity;
    public StringField $server1_password;
    public StringField $server1_base_dn;
    public StringField $server1_filter;
    public StringField $server1_base_filter;
    public IntegerField $server1_ldap_connections_number;
    public IntegerField $server1_timeout;
    public IntegerField $server1_time_limit;
    public IntegerField $server1_net_timeout;
    public StringField $server1_msad_compatibility_enable;
    public BooleanField $server1_misc_enable;
    public StringField $server1_default_profile;
    public StringField $server1_profile_attribute;
    public StringField $server1_access_attribute;
    public BooleanField $server1_group_enable;
    public StringField $server1_groupname_attribute;
    public StringField $server1_group_membership_filter;
    public StringField $server1_group_membership_attribute;
    public StringField $server1_compare_check_items;
    public StringField $server1_do_xlat;
    public StringField $server1_access_attr_used_for_allow;
    public IntegerField $server1_keepalive_idle;
    public IntegerField $server1_keepalive_probes;
    public IntegerField $server1_keepalive_interval;
    public BooleanField $server1_enable_tls_support;
    public BooleanField $server1_enable_starttls;
    public ForeignModelField $server1_ssl_ca_cert;
    public ForeignModelField $server1_ssl_server_cert;
    public StringField $server1_require_cert;

    # Server 2
    public BooleanField $server2_enable_authorize;
    public BooleanField $server2_enable_authenticate;
    public StringField $server2_server;
    public PortField $server2_server_port;
    public StringField $server2_identity;
    public StringField $server2_password;
    public StringField $server2_base_dn;
    public StringField $server2_filter;
    public StringField $server2_base_filter;
    public IntegerField $server2_ldap_connections_number;
    public IntegerField $server2_timeout;
    public IntegerField $server2_time_limit;
    public IntegerField $server2_net_timeout;
    public StringField $server2_msad_compatibility_enable;
    public BooleanField $server2_misc_enable;
    public StringField $server2_default_profile;
    public StringField $server2_profile_attribute;
    public StringField $server2_access_attribute;
    public BooleanField $server2_group_enable;
    public StringField $server2_groupname_attribute;
    public StringField $server2_group_membership_filter;
    public StringField $server2_group_membership_attribute;
    public StringField $server2_compare_check_items;
    public StringField $server2_do_xlat;
    public StringField $server2_access_attr_used_for_allow;
    public IntegerField $server2_keepalive_idle;
    public IntegerField $server2_keepalive_probes;
    public IntegerField $server2_keepalive_interval;
    public BooleanField $server2_enable_tls_support;
    public BooleanField $server2_enable_starttls;
    public ForeignModelField $server2_ssl_ca_cert;
    public ForeignModelField $server2_ssl_server_cert;
    public StringField $server2_require_cert;

    public function __construct(mixed $id = null, mixed $parent_id = null, mixed $data = [], mixed ...$options) {
        # Set model attributes - LDAP config is a singleton stored at config index 0
        $this->packages = ['pfSense-pkg-freeradius3'];
        $this->package_includes = ['freeradius.inc'];
        $this->config_path = 'installedpackages/freeradiusmodulesldap/config/0';
        $this->many = false;
        $this->always_apply = true;
        $this->verbose_name = 'FreeRADIUS LDAP';

        # Failover/load balancing mode applies to the pair of servers, stored under the server2 prefix upstream
        $this->failover_mode = new StringField(
            required: false,
            default: 'redundant',
            choices: ['redundant', 'load-balance'],
            verbose_name: 'Failover Mode',
            internal_name: 'varmodulesldap2failover',
            help_text: 'The failover/load-balancing mode used between the two LDAP servers.',
        );

        # Build server1 and server2 Field sets via the shared helper
        $this->init_ldap_server_fields(prefix: 'server1', suffix: '');
        $this->init_ldap_server_fields(prefix: 'server2', suffix: '2');

        parent::__construct($id, $parent_id, $data, ...$options);
    }

    /**
     * Defines the per-LDAP-server Field set for either server1 or server2. The two server blocks have
     * an identical set of options upstream — only the property prefix and the internal config-key suffix
     * differ — so we centralize the definitions here.
     *
     * @param string $prefix The model property prefix (e.g. `server1` or `server2`).
     * @param string $suffix The pfSense internal config-key suffix (`''` for server1, `'2'` for server2)
     *      that is appended to the literal `varmodulesldap` prefix used by the upstream package.
     */
    private function init_ldap_server_fields(string $prefix, string $suffix): void {
        $cap = $prefix === 'server1' ? 'Server 1' : 'Server 2';

        $this->{"{$prefix}_enable_authorize"} = new BooleanField(
            required: false,
            default: false,
            indicates_true: 'on',
            indicates_false: '',
            verbose_name: "$cap Enable Authorize",
            internal_name: "varmodulesldap{$suffix}enableauthorize",
            help_text: "Enables LDAP authorization support for $cap.",
        );
        $this->{"{$prefix}_enable_authenticate"} = new BooleanField(
            required: false,
            default: false,
            indicates_true: 'on',
            indicates_false: '',
            verbose_name: "$cap Enable Authenticate",
            internal_name: "varmodulesldap{$suffix}enableauthenticate",
            help_text: "Enables LDAP authentication support for $cap.",
        );
        $this->{"{$prefix}_server"} = new StringField(
            required: false,
            default: 'ldap.example.com',
            allow_empty: true,
            verbose_name: "$cap Server Address",
            internal_name: "varmodulesldap{$suffix}server",
            help_text: "The FQDN or IP address of $cap.",
        );
        $this->{"{$prefix}_server_port"} = new PortField(
            required: false,
            default: '389',
            verbose_name: "$cap Server Port",
            internal_name: "varmodulesldap{$suffix}serverport",
            help_text: "The TCP port used to connect to $cap.",
        );
        $this->{"{$prefix}_identity"} = new StringField(
            required: false,
            default: 'cn=admin,o=My Company Ltd,c=US',
            allow_empty: true,
            verbose_name: "$cap Identity",
            internal_name: "varmodulesldap{$suffix}identity",
            help_text: "The bind DN used to connect to $cap.",
        );
        $this->{"{$prefix}_password"} = new StringField(
            required: false,
            default: '',
            allow_empty: true,
            sensitive: true,
            verbose_name: "$cap Password",
            internal_name: "varmodulesldap{$suffix}password",
            help_text: "The password used to bind to $cap.",
        );
        $this->{"{$prefix}_base_dn"} = new StringField(
            required: false,
            default: 'o=My Company Ltd,c=US',
            allow_empty: true,
            verbose_name: "$cap Base DN",
            internal_name: "varmodulesldap{$suffix}basedn",
            help_text: "The base DN used when searching $cap.",
        );
        $this->{"{$prefix}_filter"} = new StringField(
            required: false,
            default: '(uid=%{%{Stripped-User-Name}:-%{User-Name}})',
            allow_empty: true,
            verbose_name: "$cap Filter",
            internal_name: "varmodulesldap{$suffix}filter",
            help_text: "The LDAP search filter used by $cap.",
        );
        $this->{"{$prefix}_base_filter"} = new StringField(
            required: false,
            default: '(objectclass=radiusprofile)',
            allow_empty: true,
            verbose_name: "$cap Base Filter",
            internal_name: "varmodulesldap{$suffix}basefilter",
            help_text: "The base LDAP filter used by $cap.",
        );
        $this->{"{$prefix}_ldap_connections_number"} = new IntegerField(
            required: false,
            default: 5,
            minimum: 1,
            verbose_name: "$cap Connections Number",
            internal_name: "varmodulesldap{$suffix}ldapconnectionsnumber",
            help_text: "The maximum number of LDAP connections opened to $cap.",
        );
        $this->{"{$prefix}_timeout"} = new IntegerField(
            required: false,
            default: 4,
            minimum: 1,
            verbose_name: "$cap Timeout",
            internal_name: "varmodulesldap{$suffix}timeout",
            help_text: "The bind/operation timeout (in seconds) used with $cap.",
        );
        $this->{"{$prefix}_time_limit"} = new IntegerField(
            required: false,
            default: 3,
            minimum: 1,
            verbose_name: "$cap Time Limit",
            internal_name: "varmodulesldap{$suffix}timelimit",
            help_text: "The LDAP search time limit (in seconds) used with $cap.",
        );
        $this->{"{$prefix}_net_timeout"} = new IntegerField(
            required: false,
            default: 1,
            minimum: 1,
            verbose_name: "$cap Network Timeout",
            internal_name: "varmodulesldap{$suffix}nettimeout",
            help_text: "The TCP network timeout (in seconds) used with $cap.",
        );
        $this->{"{$prefix}_msad_compatibility_enable"} = new StringField(
            required: false,
            default: 'Disable',
            choices: ['Disable', 'Enable'],
            verbose_name: "$cap MS Active Directory Compatibility",
            internal_name: "varmodulesldap{$suffix}msadcompatibilityenable",
            help_text: "Enables Microsoft Active Directory compatibility mode for $cap.",
        );
        $this->{"{$prefix}_misc_enable"} = new BooleanField(
            required: false,
            default: false,
            indicates_true: 'on',
            indicates_false: '',
            verbose_name: "$cap Misc Configuration",
            internal_name: "varmodulesldap{$suffix}dmiscenable",
            help_text: 'Enables the miscellaneous (Default Profile / Profile Attribute / Access Attribute) ' .
                "block for $cap.",
        );
        $this->{"{$prefix}_default_profile"} = new StringField(
            required: false,
            default: 'cn=radprofile,ou=dialup,o=My Company Ltd,c=US',
            allow_empty: true,
            verbose_name: "$cap Default Profile",
            internal_name: "varmodulesldap{$suffix}defaultprofile",
            help_text: "The default profile DN used by $cap.",
        );
        $this->{"{$prefix}_profile_attribute"} = new StringField(
            required: false,
            default: 'radiusProfileDn',
            allow_empty: true,
            verbose_name: "$cap Profile Attribute",
            internal_name: "varmodulesldap{$suffix}profileattribute",
            help_text: "The LDAP profile attribute used by $cap.",
        );
        $this->{"{$prefix}_access_attribute"} = new StringField(
            required: false,
            default: 'dialupAccess',
            allow_empty: true,
            verbose_name: "$cap Access Attribute",
            internal_name: "varmodulesldap{$suffix}accessattr",
            help_text: "The LDAP access attribute used by $cap.",
        );
        $this->{"{$prefix}_group_enable"} = new BooleanField(
            required: false,
            default: false,
            indicates_true: 'on',
            indicates_false: '',
            verbose_name: "$cap Group Enable",
            internal_name: "varmodulesldap{$suffix}groupenable",
            help_text: "Enables LDAP group membership lookups for $cap.",
        );
        $this->{"{$prefix}_groupname_attribute"} = new StringField(
            required: false,
            default: 'cn',
            allow_empty: true,
            verbose_name: "$cap Groupname Attribute",
            internal_name: "varmodulesldap{$suffix}groupnameattribute",
            help_text: "The LDAP group name attribute used by $cap.",
        );
        $this->{"{$prefix}_group_membership_filter"} = new StringField(
            required: false,
            default: '(|(&(objectClass=GroupOfNames)(member=%{control:Ldap-UserDn}))' .
                '(&(objectClass=GroupOfUniqueNames)(uniquemember=%{control:Ldap-UserDn})))',
            allow_empty: true,
            verbose_name: "$cap Group Membership Filter",
            internal_name: "varmodulesldap{$suffix}groupmembershipfilter",
            help_text: "The LDAP group membership filter used by $cap.",
        );
        $this->{"{$prefix}_group_membership_attribute"} = new StringField(
            required: false,
            default: 'radiusGroupName',
            allow_empty: true,
            verbose_name: "$cap Group Membership Attribute",
            internal_name: "varmodulesldap{$suffix}groupmembershipattribute",
            help_text: "The LDAP group membership attribute used by $cap.",
        );
        $this->{"{$prefix}_compare_check_items"} = new StringField(
            required: false,
            default: 'Yes',
            choices: ['Yes', 'No'],
            verbose_name: "$cap Compare Check Items",
            internal_name: "varmodulesldap{$suffix}comparecheckitems",
            help_text: "Whether $cap should compare check items.",
        );
        $this->{"{$prefix}_do_xlat"} = new StringField(
            required: false,
            default: 'Yes',
            choices: ['Yes', 'No'],
            verbose_name: "$cap Do XLAT",
            internal_name: "varmodulesldap{$suffix}doxlat",
            help_text: "Whether $cap should perform attribute translation (xlat).",
        );
        $this->{"{$prefix}_access_attr_used_for_allow"} = new StringField(
            required: false,
            default: 'Yes',
            choices: ['Yes', 'No'],
            verbose_name: "$cap Access Attribute Used For Allow",
            internal_name: "varmodulesldap{$suffix}accessattrusedforallow",
            help_text: "Whether the access attribute is used to allow ($cap).",
        );
        $this->{"{$prefix}_keepalive_idle"} = new IntegerField(
            required: false,
            default: 60,
            minimum: 1,
            verbose_name: "$cap KeepAlive Idle",
            internal_name: "varmodulesldap{$suffix}keepaliveidle",
            help_text: "The TCP KeepAlive idle interval (in seconds) used with $cap.",
        );
        $this->{"{$prefix}_keepalive_probes"} = new IntegerField(
            required: false,
            default: 3,
            minimum: 1,
            verbose_name: "$cap KeepAlive Probes",
            internal_name: "varmodulesldap{$suffix}keepaliveprobes",
            help_text: "The number of TCP KeepAlive probes used with $cap.",
        );
        $this->{"{$prefix}_keepalive_interval"} = new IntegerField(
            required: false,
            default: 3,
            minimum: 1,
            verbose_name: "$cap KeepAlive Interval",
            internal_name: "varmodulesldap{$suffix}keepaliveinterval",
            help_text: "The TCP KeepAlive interval (in seconds) used with $cap.",
        );
        $this->{"{$prefix}_enable_tls_support"} = new BooleanField(
            required: false,
            default: false,
            indicates_true: 'on',
            indicates_false: '',
            verbose_name: "$cap Enable TLS Support",
            internal_name: "varmodulesldap{$suffix}enabletlssupport",
            help_text: "Enables TLS support when connecting to $cap.",
        );
        $this->{"{$prefix}_enable_starttls"} = new BooleanField(
            required: false,
            default: false,
            indicates_true: 'on',
            indicates_false: '',
            verbose_name: "$cap Enable STARTTLS",
            internal_name: "varmodulesldap{$suffix}enablestarttls",
            help_text: "Use STARTTLS when establishing a TLS-protected connection to $cap.",
        );
        $this->{"{$prefix}_ssl_ca_cert"} = new ForeignModelField(
            model_name: 'CertificateAuthority',
            model_field: 'refid',
            required: false,
            default: null,
            allow_empty: true,
            allow_null: true,
            verbose_name: "$cap SSL CA Certificate",
            internal_name: "ssl_ca_cert{$suffix}",
            help_text: "The Certificate Authority (refid) used to verify the $cap server certificate when " .
                'TLS support is enabled.',
        );
        $this->{"{$prefix}_ssl_server_cert"} = new ForeignModelField(
            model_name: 'Certificate',
            model_field: 'refid',
            required: false,
            default: null,
            allow_empty: true,
            allow_null: true,
            verbose_name: "$cap SSL Server Certificate",
            internal_name: "ssl_server_cert{$suffix}",
            help_text: "The client Certificate (refid) presented when establishing a TLS connection to $cap.",
        );
        $this->{"{$prefix}_require_cert"} = new StringField(
            required: false,
            default: 'never',
            choices: ['never', 'allow', 'try', 'demand', 'hard'],
            verbose_name: "$cap Require Certificate",
            internal_name: "varmodulesldap{$suffix}requirecert",
            help_text: "The certificate verification mode used when connecting to $cap with TLS support.",
        );
    }

    /**
     * Validates the server1 server address — accepts either an IP address or a hostname/FQDN.
     */
    public function validate_server1_server(string $value): string {
        return $this->validate_ldap_server_address($value, 1);
    }

    /**
     * Validates the server2 server address — accepts either an IP address or a hostname/FQDN.
     */
    public function validate_server2_server(string $value): string {
        return $this->validate_ldap_server_address($value, 2);
    }

    /**
     * Shared check for the LDAP server address fields. Accepts a value that is either an IP address
     * or a DNS hostname/FQDN — empty values are allowed (the server is not required unless the
     * corresponding `enable_authorize` flag is set, which the upstream package's validator enforces).
     */
    private function validate_ldap_server_address(string $value, int $server_number): string {
        if ($value === '' or is_ipaddr($value) or is_hostname($value)) {
            return $value;
        }
        throw new ValidationError(
            message: "The 'server' field for LDAP Server $server_number must contain a valid IP address or hostname.",
            response_id: 'FREERADIUS_LDAP_INVALID_SERVER_ADDRESS',
        );
    }

    /**
     * Performs additional validation on the Model's fields and data by delegating to the
     * upstream pfSense FreeRADIUS validation function.
     */
    public function validate_extra(): void {
        # Run service-level validations against the upstream package's validator
        $input_errors = [];
        $ldap = $this->to_internal();
        freeradius_validate_ldap($ldap, $input_errors);

        # If there were validation errors that were not caught by the model fields, throw a ValidationError.
        if (!empty($input_errors)) {
            throw new ValidationError(
                message: "An unexpected validation error has occurred: $input_errors[0]. Please report this issue at " .
                    'https://github.com/pfrest/pfSense-pkg-RESTAPI/issues/new',
                response_id: 'FREERADIUS_LDAP_UNEXPECTED_VALIDATION_ERROR',
            );
        }
    }

    /**
     * Apply the changes made to this Model by re-rendering the FreeRADIUS LDAP module configuration.
     */
    public function apply(): void {
        freeradius_modulesldap_resync();
    }
}
