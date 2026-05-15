<?php

namespace RESTAPI\Tests;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\TestCase;
use RESTAPI\Models\RESTAPISettings;

/**
 * Defines test cases for the RESTAPISettings Model.
 */
class APIModelsRESTAPISettingsTestCase extends TestCase {
    /**
     * Checks that the `allow_development_packages` field defaults to `false`.
     */
    public function test_allow_development_packages_default_is_false(): void {
        $settings = new RESTAPISettings();
        $this->assert_is_false($settings->allow_development_packages->value);
    }

    /**
     * Checks that the `allow_development_packages` field can be set to `true` and persisted.
     */
    public function test_allow_development_packages_can_be_enabled(): void {
        # Enable the setting
        $settings = new RESTAPISettings(data: ['allow_development_packages' => true]);
        $settings->validate();
        $this->assert_is_true($settings->allow_development_packages->value);

        # Disable the setting again so we leave the system in a clean state
        $settings = new RESTAPISettings(data: ['allow_development_packages' => false]);
        $settings->validate();
        $this->assert_is_false($settings->allow_development_packages->value);
    }

    /**
     * Checks that the `allow_development_packages` field is represented as a boolean in the API response.
     */
    public function test_allow_development_packages_is_boolean_field(): void {
        $settings = new RESTAPISettings();
        $this->assert_type(value: $settings->allow_development_packages, type: 'RESTAPI\\Fields\\BooleanField');
    }
}
