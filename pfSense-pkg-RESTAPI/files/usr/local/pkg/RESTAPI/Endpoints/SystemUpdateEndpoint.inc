<?php

namespace RESTAPI\Endpoints;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Endpoint;

/**
 * Defines an Endpoint for interacting with singular SystemUpdate Model objects at /api/v2/system/update.
 */
class SystemUpdateEndpoint extends Endpoint {
    public function __construct() {
        # Set Endpoint attributes
        $this->url = '/api/v2/system/update';
        $this->model_name = 'SystemUpdate';
        $this->request_method_options = ['POST'];
        $this->post_help_text = 'Initiates a pfSense update process.';

        # Construct the parent Endpoint object
        parent::__construct();
    }
}
