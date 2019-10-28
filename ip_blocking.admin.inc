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
  $rows = array();
  $header = array(t('Blocked IP address'), t('When blocked'), t('Blocked by'), t('Reason'), t('Unblock'));
  $result = db_query('SELECT * FROM {blocked_ips}');
  foreach ($result as $ip) {
    $time = !empty($ip->time) ? format_date($ip->time, 'short') : t('N/A');
    if (!empty($ip->uid)) {
      $user = user_load($ip->uid);
      $username = $user->name;
    }
    else {
      $username = t('N/A');
    }
    $rows[] = array(
      $ip->ip,
      $time,
      $username,
      $ip->reason,
      l(t('unblock'), "admin/config/people/ip-blocking/unblock/$ip->iid"),
    );
  }

  $build['ip_blocking_form'] = backdrop_get_form('ip_blocking_form', $default_ip);

  $build['ip_blocking_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No blocked IP addresses available.'),
  );

  return $build;
}

/**
 * Define the form for blocking IP addresses.
 *
 */
function ip_blocking_form($form, $form_state, $default_ip) {
  $form['ip'] = array(
    '#title' => t('Add IP address to list blocked'),
    '#type' => 'textfield',
    '#size' => 48,
    '#maxlength' => 40,
    '#default_value' => $default_ip,
    '#description' => t('Enter a valid IP address.'),
    '#prefix' => '<p>' . t('On this page you can manage blocked IP addresses') . '</p>',
  );
  $form['reason'] = array(
    '#title' => t('Reason of blocking (optional)'),
    '#type' => 'textfield',
    '#size' => 70,
    '#maxlength' => 255,
    '#default_value' => '',
    '#description' => t('Short description of reason for blocking this IP, for example: "Drupalgeddon2 attack".'),
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
  elseif (filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_NO_RES_RANGE) == FALSE) {
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
    ->fields(array('ip' => $ip, 'reason' => $reason, 'time' => $time, 'uid' => $uid))
    ->execute();
  
  backdrop_set_message(t('The IP address %ip has been blocked.', array('%ip' => $ip)));
  $form_state['redirect'] = 'admin/config/people/ip-blocking';
  return;
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
