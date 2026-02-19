<?php
/**
 * @file
 * Admin forms to manage module
 *
 */

/**
 * Menu callback. Display blocked IP addresses.
 *
 * @param $default_ip Optional IP address to be passed on to backdrop_get_form()
 * for use as the default value of the IP address form field.
 */
function ip_blocking($default_ip = '') {
  $number_of_items = config_get('ip_blocking.settings', 'number_of_items');
  $rows   = array();
  $header = array(
    array('data' => t('Blocked IP'), 'field' => 'ip'),
    array('data' => t('Time'),       'field' => 'time', 'sort' => 'desc'),
    array('data' => t('Blocker'),    'field' => 'uid'),
    array('data' => t('Reason'),     'field' => 'reason'),
    array('data' => t('Operations')),
  );

  $select = db_select('blocked_ips', 'b')
              ->extend('PagerDefault')
              ->extend('TableSort');

  $select->fields('b', array('iid', 'ip', 'reason', 'uid', 'time'))
         ->limit($number_of_items)
         ->orderByHeader($header);

  $results = $select->execute();

  foreach ($results as $ip) {
    $time = !empty($ip->time) ? format_date($ip->time, 'short') : t('N/A');
    if (!empty($ip->uid)) {
      // Antiscan module
      if ($ip->uid == ANTISCAN_MODULE_UID) {
        $username = 'Antiscan module';
      }
      else {
        $user = user_load($ip->uid);
        $username = $user->name;
      }
    }
    else {
      // for IPs blocked before module ver. 1.0.3
      $username = t('N/A');
    }

    $rows[] = array(
      $ip->ip,
      $time,
      $username,
      check_plain($ip->reason),
      l(t('unblock'), "admin/config/people/ip-blocking/unblock/$ip->iid",
        array('attributes' => array('title' => 'Unblock this IP')))
      . '&nbsp;|&nbsp;' .
      l(t('check'), "https://www.abuseipdb.com/check/$ip->ip",
        array('attributes' => array('target' => '_blank', 'title' => 'Check IP status in AbuseIPDB'))),
    );
  }

  $build['ip_blocking_form'] = backdrop_get_form('ip_blocking_form', $default_ip);
  $output = backdrop_render($build);
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

/**
 * Define the form for blocking IP addresses.
 *
 */
function ip_blocking_form($form, &$form_state, $default_ip) {
  $form['#attached']['css'] = array(
    backdrop_get_path('module', 'ip_blocking') . '/css/ip_blocking.admin.css',
  );

  $form['ip'] = array(
    '#title' => t('Add IP address to list blocked'),
    '#type' => 'textfield',
    '#size' => 34,
    '#maxlength' => 40,
    '#default_value' => $default_ip,
    '#description' => t('Enter a valid IP address.'),
    '#prefix' => '<p>' . t('The IP addresses listed below are blocked from this site. Blocked addresses will be completely denied access to the site and will instead see a short message explaining the situation or a 404 error page (of your choice using the "Settings" tab above).') .
    '<br>' . t('From the "Operations" column you can unblock any IP and check its status in AbuseIPDB (opens in a new tab).') . '</p>',
  );
  $form['reason'] = array(
    '#title' => t('Reason for blocking (optional)'),
    '#type' => 'textfield',
    '#size' => 52,
    '#maxlength' => 255,
    '#default_value' => '',
    '#description' => t('Short description of the reason for blocking this IP, e.g: "Drupalgeddon2 attack".'),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );
  $form['#submit'][] = 'ip_blocking_form_submit';
  $form['#validate'][] = 'ip_blocking_form_validate';

  return $form;
}

function ip_blocking_form_validate($form, &$form_state) {
  $ip = trim($form_state['values']['ip']);

  if (db_query("SELECT * FROM {blocked_ips} WHERE ip = :ip", array(':ip' => $ip))->fetchField()) {
    form_set_error('ip', t('This IP address is already blocked.'));
  }
  elseif ($ip == ip_address()) {
    form_set_error('ip', t('You can not block your own IP address.'));
  }
  elseif (!filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_NO_RES_RANGE)) {
    form_set_error('ip', t('Enter a valid IP address.'));
  }
}

function ip_blocking_form_submit($form, &$form_state) {
  global $user;
  $ip     = trim($form_state['values']['ip']);
  $uid    = $user->uid;
  $time   = time();
  $reason = trim($form_state['values']['reason']);

  db_insert('blocked_ips')
    ->fields(array('ip' => $ip, 'reason' => $reason, 'time' => $time, 'uid' => $uid, 'type' => 'admin_form'))
    ->execute();

  backdrop_set_message(t('The IP address %ip has been blocked.', array('%ip' => $ip)));
  $form_state['redirect'] = 'admin/config/people/ip-blocking';
}

/**
 * IP unblock confirm page.
 *
 */
function ip_blocking_unblock($form, &$form_state, $iid) {
  $form['blocked_ip'] = array(
    '#type' => 'value',
    '#value' => $iid,
  );

  return confirm_form($form, t('Are you sure you want to unblock %ip?', array('%ip' => $iid['ip'])), 'admin/config/people/ip-blocking', t('This action cannot be undone.'), t('Unblock'), t('Cancel'));
}

/**
 * Process ip_blocking_unblock form submissions.
 */
function ip_blocking_unblock_submit($form, &$form_state) {
  $blocked_ip = $form_state['values']['blocked_ip'];

  db_delete('blocked_ips')
    ->condition('iid', $blocked_ip['iid'])
    ->execute();

  watchdog('ip_blocking', 'Unblocked IP %ip', array('%ip' => $blocked_ip['ip']));
  backdrop_set_message(t('The IP address %ip was unblocked.', array('%ip' => $blocked_ip['ip'])));
  $form_state['redirect'] = 'admin/config/people/ip-blocking';
}

/**
 * Form for blocking IP additional settings.
 *
 */
function ip_blocking_settings_form($form, &$form_state) {
  $form['number_of_items'] = array(
    '#type' => 'select',
    '#title' => t('Blocked IPs table pager'),
    '#options' => array(
      25 => t('25 items per page'),
      50 => t('50 items per page'),
      75 => t('75 items per page'),
      100 => t('100 items per page'),
    ),
    '#default_value' => config_get('ip_blocking.settings', 'number_of_items'),
    '#description' => t('Display a specified number of items per page in the blocked IPs table.'),
  );

  $form['log_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable logging'),
    '#default_value' => config_get('ip_blocking.settings', 'log_enabled'),
    '#description' => t('Enable logging of access attempts from blocked IPs and actions of this module.'),
  );

  $form['return_404'] = array(
    '#type' => 'checkbox',
    '#title' => t('Return code 404'),
    '#default_value' => config_get('ip_blocking.settings', 'return_404'),
    '#description' => t('Return a 404 (Not Found) status code for visitors from blocked IP instead of the default 403 (Access Denied).'),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );

  return $form;
}

/**
 * Process ip_blocking_settings_form form submissions.
 */
function ip_blocking_settings_form_submit($form, &$form_state) {
  $config = config('ip_blocking.settings');

  $config->set('number_of_items', $form_state['values']['number_of_items']);
  $config->set('log_enabled',     $form_state['values']['log_enabled']);
  $config->set('return_404',      $form_state['values']['return_404']);
  $config->save();

  backdrop_set_message(t('The configuration options have been saved.'));
}

/**
 * Define the form for dealing with orphaned table.
 *
 */
function ip_blocking_orphaned_table_form($form, &$form_state) {
  if (db_table_exists('blocked_ips_d7')) {
    $count_ips = db_select('blocked_ips_d7')
      ->countQuery()
      ->execute()
      ->fetchField();

    $form['info'] = array(
      '#markup' =>  '<p>' . t('Currently database table named "blocked_ips_d7" contains @count_ips records.',
        array('@count_ips' => $count_ips)) . '</p>',
    );

    if ($count_ips > 0) {
      $form['not_empty'] = array(
        '#type' => 'help',
        '#markup' =>t('Use the buttons below to import inherited data and then delete the orphaned table, or delete the table without importing any data.'),
      );

      $form['reason'] = array(
        '#title' => t('Optional: blocking reason for all imported records'),
        '#type' => 'textfield',
        '#maxlength' => 255,
        '#default_value' => '',
        '#description' => t('If this field is left blank, the value "Imported during migration" is used by default.'),
      );

      $form['actions'] = array('#type' => 'actions');
      $form['actions']['import'] = array(
        '#type' => 'submit',
        '#value' => t('Import data then delete table'),
      );
      $form['actions']['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete table without import'),
        '#attributes' => array('class' => array('button-danger')),
      );
    }
    else {
      $form['empty'] = array(
        '#type' => 'help',
        '#markup' =>t('You can safely delete an empty table.'),
      );

      $form['actions'] = array('#type' => 'actions');
      $form['actions']['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete table'),
      );
    }
    return $form;
  }
}

/**
 * Process ip_blocking_orphaned_table_form form submissions.
 */
function ip_blocking_orphaned_table_form_submit($form, &$form_state) {
  global $user;
  $uid = $user->uid;
  $time = time();
  $reason = 'Imported during migration';

  if ($form_state['triggering_element']['#id'] == 'edit-import') {
    if (!empty(trim($form_state['values']['reason']))) {
      $reason = check_plain(trim($form_state['values']['reason']));
    }

    $transaction = db_transaction();
    try {
      $result = db_select('blocked_ips_d7', 'd7')
        ->fields('d7', array('ip'))
        ->execute();
      $select = array();

      foreach ($result as $row) {
        $select[] = array(
          'ip' => $row->ip,
          'uid' => $uid,
          'reason' => $reason,
          'time' => $time,
        );
      }

      $query = db_insert('blocked_ips')
        ->fields(array('ip', 'uid', 'reason', 'time'));
      foreach ($select as $record) {
        $query->values($record);
      }
      $query->execute();

      db_drop_table('blocked_ips_d7');
      $message = t('Data from table "blocked_ips_d7" successfully imported, table deleted.');
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('ip_blocking_import', $e);
    }
  }
  else {
    db_drop_table('blocked_ips_d7');
    $message = t('Database table "blocked_ips_d7" has been deleted.');
  }
  backdrop_flush_all_caches();
  backdrop_set_message($message);
  $form_state['redirect'] = 'admin/config/people/ip-blocking';
}

function ip_blocking_ranges_page() {
  $config = config('ip_blocking.settings');
  $number_of_items = (int) $config->get('number_of_items');
  if ($number_of_items <= 0) {
    $number_of_items = 50;
  }

  $header = array(
    array('data' => t('Range'), 'field' => 'range_label'),
    array('data' => t('Time'), 'field' => 'time', 'sort' => 'desc'),
    array('data' => t('Blocker'), 'field' => 'uid'),
    array('data' => t('Reason'), 'field' => 'reason'),
    array('data' => t('Operations')),
  );

  $rows = array();

  if (db_table_exists('blocked_ip_ranges')) {
    $select = db_select('blocked_ip_ranges', 'r')
      ->extend('PagerDefault')
      ->extend('TableSort');

    $select->fields('r', array('rid', 'range_label', 'reason', 'uid', 'time'))
      ->limit($number_of_items)
      ->orderByHeader($header);

    $results = $select->execute();

    foreach ($results as $r) {
      $time = !empty($r->time) ? format_date($r->time, 'short') : t('N/A');

      if (!empty($r->uid)) {
        if (defined('ANTISCAN_MODULE_UID') && $r->uid == ANTISCAN_MODULE_UID) {
          $username = t('Antiscan module');
        }
        else {
          $account = user_load($r->uid);
          $username = $account ? $account->name : t('N/A');
        }
      }
      else {
        $username = t('N/A');
      }

      $ops = l(
        t('unblock'),
        "admin/config/people/ip-blocking/unblock_range/{$r->rid}",
        array('attributes' => array('title' => t('Unblock this range'))))
        . '&nbsp;|&nbsp;' .
        l(t('check'), "https://www.abuseipdb.com/check-block/{$r->range_label}",
          array('attributes' => array('target' => '_blank', 'title' => 'Check IP range status in AbuseIPDB')));

      $rows[] = array(
        check_plain($r->range_label),
        $time,
        check_plain($username),
        check_plain($r->reason),
        $ops,
      );
    }
  }

  $build = array();
  $build['range_form'] = backdrop_get_form('ip_blocking_ranges_form');

  $output = backdrop_render($build);
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

function ip_blocking_ranges_form($form, &$form_state) {
  $form['#attached']['css'] = array(
    backdrop_get_path('module', 'ip_blocking') . '/css/ip_blocking.admin.css',
  );

  $form['range_label'] = array(
    '#title' => t('Add IP range to blocked list'),
    '#type' => 'textfield',
    '#size' => 48,
    '#maxlength' => 64,
    '#default_value' => '',
    '#description' => t('Enter a CIDR (e.g. 1.2.3.0/24 or 2001:db8::/32)')
      . '<br>'
      . t('or an explicit range (e.g. 1.2.3.4-1.2.3.9).'),
    '#prefix' => '<p>' . t('The IP ranges listed below are blocked from this site. Blocked IP ranges will be completely denied access to the site and will instead see a short message explaining the situation or a 404 error page (of your choice using the "Settings" tab above).') .
      '<br>' . t('From the "Operations" column you can unblock any IP range and check its status in AbuseIPDB (opens in a new tab).') . '</p>',
  );

  $form['reason'] = array(
    '#title' => t('Reason for blocking (optional)'),
    '#type' => 'textfield',
    '#size' => 52,
    '#maxlength' => 255,
    '#default_value' => '',
    '#description' => t('Short description of the reason for blocking this IP range, e.g: "DDoS subnet".'),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );

  $form['#validate'][] = 'ip_blocking_ranges_form_validate';
  $form['#submit'][] = 'ip_blocking_ranges_form_submit';

  return $form;
}

function ip_blocking_ranges_form_validate($form, &$form_state) {
  $range_label = trim($form_state['values']['range_label']);
  $parsed = ip_blocking_parse_range($range_label);
  if (!$parsed) {
    form_set_error('range_label', t('Enter a valid CIDR or IP range.'));
    return;
  }

  $my_ip = ip_address();
  $my_info = ip_blocking_ip_to_bin($my_ip);
  if ($my_info && $my_info['family'] == $parsed['family']) {
    if (strcmp($parsed['start_bin'], $my_info['bin']) <= 0 && strcmp($parsed['end_bin'], $my_info['bin']) >= 0) {
      form_set_error('range_label', t('You can not block your own IP address.'));
      return;
    }
  }

  if (!db_table_exists('blocked_ip_ranges')) {
    form_set_error('range_label', t('Range blocking is not available (missing database table). Run update.php.'));
    return;
  }

  $exists = db_query(
    "SELECT 1 FROM {blocked_ip_ranges} WHERE range_label = :range_label",
    array(':range_label' => $parsed['label'])
  )->fetchField();

  if ($exists) {
    form_set_error('range_label', t('This IP range is already blocked.'));
    return;
  }

  $form_state['ip_blocking_parsed_range'] = $parsed;
}

function ip_blocking_ranges_form_submit($form, &$form_state) {
  global $user;

  $parsed = $form_state['ip_blocking_parsed_range'];
  $reason = trim($form_state['values']['reason']);

  db_insert('blocked_ip_ranges')->fields(array(
    'ip_family' => (int) $parsed['family'],
    'ip_start' => $parsed['start_bin'],
    'ip_end' => $parsed['end_bin'],
    'range_label' => $parsed['label'],
    'reason' => $reason,
    'uid' => (int) $user->uid,
    'time' => time(),
    'type' => 'admin_form',
  ))->execute();

  backdrop_set_message(t('The range %range has been blocked.', array('%range' => $parsed['label'])));
  $form_state['redirect'] = 'admin/config/people/ip-blocking/ranges';
}

function ip_blocking_unblock_range($form, &$form_state, $rid) {
  $row = db_query(
    "SELECT rid, range_label FROM {blocked_ip_ranges} WHERE rid = :rid",
    array(':rid' => (int) $rid)
  )->fetchAssoc();

  if (!$row) {
    backdrop_set_message(t('Range not found.'), 'error');
    $form_state['redirect'] = 'admin/config/people/ip-blocking/ranges';
    return array();
  }

  $form['rid'] = array('#type' => 'value', '#value' => (int) $row['rid']);
  $form['#submit'][] = 'ip_blocking_unblock_range_submit';

  return confirm_form(
    $form,
    t('Are you sure you want to unblock %range?', array('%range' => $row['range_label'])),
    'admin/config/people/ip-blocking/ranges',
    t('This action cannot be undone.'),
    t('Unblock'),
    t('Cancel')
  );
}

function ip_blocking_unblock_range_submit($form, &$form_state) {
  $rid = (int) $form_state['values']['rid'];
  db_delete('blocked_ip_ranges')->condition('rid', $rid)->execute();

  backdrop_set_message(t('The range has been unblocked.'));
  $form_state['redirect'] = 'admin/config/people/ip-blocking/ranges';
}
