<?php

/**
 * @file
 * Drush command to set site environment.
 */

/**
 * Implements hook_drush_command().
 */
function set_environment_drush_command() {
  $items['set-environment'] = array(
    'description' => 'Set site environment',
    'aliases' => array('se'),
  );
  return $items;
}

/**
 * Command callback: Set site deployment state.
 *
 * @TODO Support more than two environments.
 */
function drush_set_environment($environment = FALSE) {

  if ($environment && $environment != 'production' && $environment != 'development') {
    drush_set_error('Invalid site environment value!');
    $environment = FALSE;
  }

  if (!$environment) {
    $environment = drush_choice(
      array('production' => 'Production', 'development' => 'Development'),
      'Select site environment:'
    );
    // Canceled.
    if (!$environment) {
      return;
    }
  }


  $previous_state = $environment == 'production' ? 'development' : 'production';
  drush_print("Now we will put site to $environment environment...");

  drush_include_engine('drupal', 'environment');

  $step = 0;

  $step++;
  drush_print("\n$step. Disable $previous_state modules...");
  call_user_func_array('drush_pm_disable', set_environment_get_modules($previous_state));

  $step++;
  drush_print("\n$step. Enable $environment modules...");
  drush_set_context('PM_ENABLE_MODULES', set_environment_get_modules($environment));
  drush_pm_enable();

  $step++;
  drush_print("\n$step. Set drupal variables to $environment state...");
  foreach (set_environment_get_variables($environment) as $variable_name => $variable_value) {
    drush_print("   $variable_name = $variable_value");
    variable_set($variable_name, $variable_value);
  }

  $step++;
  drush_print("\n$step. Clear dblog messages...");
  db_query('DELETE FROM {watchdog}');

  $step++;
  drush_print("\n$step. Invoke \"hook_set_environment\"...");

  module_invoke_all('set_environment', $environment);

  $step++;
  drush_print("\n$step. Run all cron hooks...");
  drush_core_cron();

  $step++;
  drush_print("\n$step. Flush all caches...");
  drupal_flush_all_caches();

  // Finish.
  watchdog('site-environment', "Site moved to $environment environment", array(), WATCHDOG_NOTICE, l('View status report', 'admin/reports/status'));
}

/**
 * List of modules which should be enabled or disabled.
 *
 * @TODO Move module list to settings.php
 */
function set_environment_get_modules($environment) {
  $modules = array(
    'production' => array(
      //'googleanalytics',
    ),
    'development' => array(
      'devel',
      'devel_generate',
      'devel_node_access',
      'menu',
      'views_ui',
      'field_ui',
      'configuration_ui',
      'l10n_update',
      'diff',
      'simpletest',
      'update',
    ),
  );
  return $modules[$environment];
}

/**
 * List of processed variables.
 *
 * @TODO Move variables list to settings.php
 */
function set_environment_get_variables($environment) {
  $variables =  array(
    'production' => array (
      'cache' => 1,
      'block_cache' => 1,
      'cache_lifetime' => 86400, 
      'preprocess_js' => 1,
      'preprocess_css' => 1,
      'error_level' => 0,
    ),
    'development' => array (
      'cache' => 0,
      'block_cache' => 0,
      'cache_lifetime' => 0, 
      'preprocess_js' => 0,
      'preprocess_css' =>  0,
      'error_level' => 2,
    ),
  );
  return $variables[$environment];
}
