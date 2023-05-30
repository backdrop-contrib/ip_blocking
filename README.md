IP address blocking
===================

Allows you to block IP addresses.  
This module restores the lost functionality of the Drupal core. It comes with a few improvements.

You can:
 - see when an IP was blocked, who blocked that IP and the reason for the block (if specified);
 - set a 404 (Not Found) status code for visitors from blocked IPs instead of the default 403 (Access Denied);
 - enable logging of access attempts from blocked IPs;
 - from the "Recent log messages" event pages (admin/reports/event/EVENT_NUMBER) you can easily block or unblock an IP
   and check the status of this IP on the [AbuseIPDB](https://www.abuseipdb.com).

New in version 1.x-1.0.5:
 - display of the number of blocked IPs on the "Status report" page;
 - integration with the new "Antiscan" module (https://backdropcms.org/project/antiscan) to
   automatically block IP addresses used by bad crawlers or vulnerability scanners.

Installation
------------
Install this module using the official Backdrop CMS instructions at https://backdropcms.org/guide/modules

Note: if the "Ban IP" module is installed, you must to uninstall it first to avoid confusion when using the same
but extended database table.

Configuration and usage
-----------------------
The administration page is available from the *Administration > Configuration >
User accounts > IP address blocking* menu (admin/config/people/ip-blocking)
and can be used to:

- block an IP address:
    - enter a valid IP address (e.g. 10.0.0.1);
    - (optional) enter description of the reason for blocking this IP;
    - click Add;

- unblock a previously blocked IP address:
    - click "unblock" next to an IP address and confirm the unblocking.

While browsing "Recent log messages" (admin/reports/dblog) you can quickly review
an individual entry (admin/reports/event/EVENT_NUMBER) and block (or unblock)
an IP address and check the status of this IP on [AbuseIPDB](https://www.abuseipdb.com) from the "Operation" links.

This link is displayed for 'access denied', 'antiscan', 'ip_blocking', 'login_allowlist',
'page not found', 'system', 'user' and 'php' events only if the event has a valid IP address and not the IP address of the currently logged in user.

**Screenshots** are available at https://findlab.net/projects/ip-address-blocking

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
