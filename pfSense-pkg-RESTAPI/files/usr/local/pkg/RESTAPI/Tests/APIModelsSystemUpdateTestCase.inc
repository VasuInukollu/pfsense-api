<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Models\SystemUpdate;

class APIModelsSystemUpdateTestCase extends TestCase {
    /**
     * Check that the SystemUpdate model accepts and validates dry_run inputs.
     */
    public function test_validate_dry_run_field(): void {
        $this->assert_does_not_throw(
            callable: function () {
                $update = new SystemUpdate(dry_run: true);
                $this->assert_is_true($update->dry_run);
                $update->create();
            },
        );
    }
}
