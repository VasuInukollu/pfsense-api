<?php

namespace RESTAPI\Models;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Model;
use RESTAPI\Dispatchers\SystemUpdateDispatcher;
use RESTAPI\Fields\BooleanField;

/**
 * Defines a Model that performs a pfSense update operation.
 */
class SystemUpdate extends Model {
    public BooleanField $dry_run;

    public function __construct(mixed $id = null, mixed $parent_id = null, mixed $data = [], ...$options) {
        # Set model attributes
        $this->many = false;
        $this->always_apply = true;

        # Set model fields
        $this->dry_run = new BooleanField(
            default: false,
            write_only: true,
            verbose_name: 'Dry Run',
            help_text: 'Run the pfSense update process in dry-run mode (-n) without applying changes.',
        );

        parent::__construct($id, $parent_id, $data, ...$options);
    }

    /**
     * This model only triggers apply behavior, so this method intentionally does nothing.
     */
    public function _create(): void {
        # Intentionally empty.
    }

    /**
     * Spawns a dispatcher process that performs package update + pfSense upgrade.
     */
    public function apply(): void {
        (new SystemUpdateDispatcher(async: $this->async))->spawn_process(dry_run: $this->dry_run->value);
    }
}
