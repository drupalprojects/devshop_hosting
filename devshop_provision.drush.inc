<?php

/**
 * @file
 * Provision/Drush hooks for git commands.
 */

/**
 * Implementation of hook_drush_command().
 */
function devshop_provision_drush_command() {
  $items['provision-devshop-pull'] = array(
    'description' => 'Pull & verify site code and (optionally) run update.php, clear cache, and revert features.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'options' => array(
      //'sync' => 'Sync content from default source site before code pull.',
      'no-update' => 'Run update.php after code pull.',
      'no-revert' => 'Revert all features after code pull.',
      'no-cache' => 'Clear all caches after code pull.',
    ),
    'aliases' => array('pdp'),
  );
  $items['provision-devshop-sync'] = array(
    'description' => 'Sync database (and files, coming soon) from a chosen source site.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'options' => array(
      'update' => 'Run update.php after content sync.',
      'revert' => 'Revert all features after content sync.',
      'cache' => 'Clear all caches after content sync.',
    ),
    'arguments' => array(
      'from' => 'Site alias to sync from.', 
      'to' => 'Site alias to sync to.',
    ), 
    'aliases' => array('pds'),
  );
  $items['provision-devshop-commit'] = array(
    'description' => 'Export the site\'s Features and commit the result.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'options' => array(
      'revert' => ' Force revert all features after exporting and committing.',
    ),
    'aliases' => array('pdc'),
    'arguments' => array(
      'from' => 'Site alias to sync from.',
    ),
  );
  return $items;
}

function drush_devshop_provision_pre_hosting_task() {
  $task =& drush_get_context('HOSTING_TASK');
  // Pull
  if ($task->ref->type == 'site' && $task->task_type == 'devshop-pull') {
    $task->options['no-update'] = !$task->task_args['update'];
    $task->options['no-revert'] = !$task->task_args['revert'];
    $task->options['no-cache'] = !$task->task_args['cache'];
  }
  
  // Commit
  if ($task->ref->type == 'site' && $task->task_type == 'devshop-commit') {
    $task->options['message'] = $task->task_args['message'];
    $task->options['push'] = $task->task_args['push'];
    $task->options['revert'] = $task->task_args['revert'];
  }
  
}