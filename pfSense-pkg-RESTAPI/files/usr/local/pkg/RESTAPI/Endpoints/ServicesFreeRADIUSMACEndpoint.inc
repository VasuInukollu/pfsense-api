<?php

namespace RESTAPI\Endpoints;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Endpoint;

/**
 * Defines an Endpoint for interacting with a single FreeRADIUSMAC Model object at
 * /api/v2/services/freeradius/mac
 */
class ServicesFreeRADIUSMACEndpoint extends Endpoint {
    public function __construct() {
        /**
         *  Set Endpoint attributes
         */
        $this->url = '/api/v2/services/freeradius/mac';
        $this->model_name = 'FreeRADIUSMAC';
        $this->many = false;
        $this->request_method_options = ['GET', 'POST', 'PATCH', 'DELETE'];

        # Construct the parent Endpoint object
        parent::__construct();
    }
}
