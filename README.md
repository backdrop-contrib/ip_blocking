IP address blocking
===================

Enables blocking of IP addresses. 
This module restores lost Drupal core functionality with some informative additions. 
You can see when IP was blocked, who of users block this IP and reason for blocking (if specified).

Installation
------------
Install this module using the official Backdrop CMS instructions at 
https://backdropcms.org/guide/modules

Configuration and usage
-----------------------
Administration page is available via menu *Administration > Configuration > 
User accounts > IP address blocking* (admin/config/people/ip-blocking) 
and may be used for:

- block an IP address:
  - enter a valid IP address (for example, 10.0.0.1);
  - (optional) enter description of reason for blocking this IP;
  - click Add;

- unblock previously blocked IP address:
  - beside an IP address, click "unblock", then confirm unblocking.

License
-------
This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

Current Maintainer
------------------
Vladimir (https://github.com/findlabnet/)

More information
----------------
For bug reports, feature or support requests, please use the module 
issue queue at https://github.com/backdrop-contrib/ip_blocking/issues.
