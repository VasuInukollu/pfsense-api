<?php

namespace RESTAPI\Models;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Model;
use RESTAPI\Fields\StringField;
use RESTAPI\ModelTraits\FreeRADIUSCommonModelTraits;
use RESTAPI\Responses\ValidationError;
use RESTAPI\Validators\MACAddressValidator;

/**
 * Defines a Model that represents FreeRADIUS authorized MAC entries.
 */
class FreeRADIUSMAC extends Model {
    use FreeRADIUSCommonModelTraits;

    public StringField $mac;

    public function __construct(mixed $id = null, mixed $parent_id = null, mixed $data = [], mixed ...$options) {
        # Set model attributes
        $this->packages = ['pfSense-pkg-freeradius3'];
        $this->package_includes = ['freeradius.inc'];
        $this->config_path = 'installedpackages/freeradiusauthorizedmacs/config';
        $this->many = true;
        $this->always_apply = true;
        $this->verbose_name = 'FreeRADIUS MAC';
        $this->verbose_name_plural = 'FreeRADIUS MACs';

        # Set the MAC-specific Fields
        $this->mac = new StringField(
            required: true,
            unique: true,
            verbose_name: 'MAC Address',
            internal_name: 'varmacsaddress',
            validators: [new MACAddressValidator()],
            help_text: "The MAC address of the entry. May be delimited with '-' or ':' (e.g. 'aa:bb:cc:dd:ee:ff').",
        );

        # Define the Fields shared with FreeRADIUSUser via the FreeRADIUSCommonModelTraits trait.
        # Note: pfSense uses `varmacsswisprredirectionurl` (with a leading 's') for the WISPr URL on
        # MAC entries, while users use `varuserswisprredirectionurl`. The url_field_suffix override
        # accommodates this upstream naming quirk.
        $this->init_freeradius_common_fields(prefix: 'varmacs', url_field_suffix: 'swisprredirectionurl');

        parent::__construct($id, $parent_id, $data, ...$options);
    }

    /**
     * Perform additional validation on the Model's fields and data.
     */
    public function validate_extra(): void {
        # Run service level validations
        $input_errors = [];
        $mac = $this->to_internal();
        freeradius_validate_macs($mac, $input_errors);

        # If there were validation errors that were not caught by the model fields, throw a ValidationError.
        # Ideally the Model should catch all validation errors itself so prompt the user to report this error.
        if (!empty($input_errors)) {
            throw new ValidationError(
                message: "An unexpected validation error has occurred: $input_errors[0]. Please report this issue at " .
                    'https://github.com/pfrest/pfSense-pkg-RESTAPI/issues/new',
                response_id: 'FREERADIUS_MAC_UNEXPECTED_VALIDATION_ERROR',
            );
        }
    }

    /**
     * Apply the changes made to this Model to the FreeRADIUS authorized_macs configuration.
     */
    public function apply(): void {
        freeradius_authorizedmacs_resync();
    }
}
