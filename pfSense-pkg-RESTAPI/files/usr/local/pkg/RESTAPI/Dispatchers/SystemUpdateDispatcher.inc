<?php

namespace RESTAPI\Dispatchers;

use RESTAPI\Core\Command;
use RESTAPI\Core\Dispatcher;

/**
 * Defines a Dispatcher for running pfSense package updates.
 */
class SystemUpdateDispatcher extends Dispatcher {
    # Only allow one upgrade process at a time
    public int $max_queue = 1;

    /**
     * Updates package metadata and runs the pfSense upgrade process.
     */
    protected function _process(mixed ...$arguments): void {
        # Accept either named or positional dry_run argument.
        $dry_run = false;
        if (array_key_exists('dry_run', $arguments)) {
            $dry_run = (bool) $arguments['dry_run'];
        } elseif (array_key_exists(0, $arguments)) {
            $dry_run = (bool) $arguments[0];
        }

        $this->run_command('/usr/local/sbin/pkg-static update');

        $upgrade_command = '/usr/local/sbin/pfSense-upgrade -y';
        if ($dry_run) {
            $upgrade_command .= ' -n';
        }

        $this->run_command($upgrade_command);
    }

    /**
     * Executes a shell command for this dispatcher process.
     */
    protected function run_command(string $command): void {
        new Command($command);
    }
}
