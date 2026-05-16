<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Dispatchers\SystemUpdateDispatcher;

class TestSystemUpdateDispatcher extends SystemUpdateDispatcher {
    public array $commands = [];

    protected function run_command(string $command): void {
        $this->commands[] = $command;
    }
}

class APIDispatchersSystemUpdateDispatcherTestCase extends TestCase {
    /**
     * Ensure the dispatcher enforces max_queue of one process.
     */
    public function test_max_queue_is_one(): void {
        $dispatcher = new SystemUpdateDispatcher();
        $this->assert_equals($dispatcher->max_queue, 1);
    }

    /**
     * Ensure the dispatcher calls pkg-static update and standard upgrade by default.
     */
    public function test_process_without_dry_run_uses_standard_upgrade_command(): void {
        $dispatcher = new TestSystemUpdateDispatcher();
        $dispatcher->process();

        $this->assert_equals($dispatcher->commands, [
            '/usr/local/sbin/pkg-static update',
            '/usr/local/sbin/pfSense-upgrade -y',
        ]);
    }

    /**
     * Ensure named dry_run appends -n to pfSense-upgrade.
     */
    public function test_process_with_named_dry_run_uses_noop_upgrade_command(): void {
        $dispatcher = new TestSystemUpdateDispatcher();
        $dispatcher->process(dry_run: true);

        $this->assert_equals($dispatcher->commands, [
            '/usr/local/sbin/pkg-static update',
            '/usr/local/sbin/pfSense-upgrade -y -n',
        ]);
    }

    /**
     * Ensure positional dry_run argument also appends -n to pfSense-upgrade.
     */
    public function test_process_with_positional_dry_run_uses_noop_upgrade_command(): void {
        $dispatcher = new TestSystemUpdateDispatcher();
        $dispatcher->process(true);

        $this->assert_equals($dispatcher->commands, [
            '/usr/local/sbin/pkg-static update',
            '/usr/local/sbin/pfSense-upgrade -y -n',
        ]);
    }
}
