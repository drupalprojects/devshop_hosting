<?php
/**
 * @file commit.inc
 * Functions related to the "Commit Features" task.
 */

/**
 * Implementation of hook_hosting_task_TASK_TYPE_form().
 *
 * For "Commit" task.
 */
function hosting_task_devshop_commit_form($node) {

  $form = array();
  $descr = 'A message describing this commit. Too see a diff output off all of the features, ';

  $form['message'] = array(
    '#title' => t('Commit Message'),
    '#type' => 'textarea',
    '#description' => $descr,
  );
  $form['revert'] = array(
    '#title' => t('Force revert features after commit?'),
    '#type' => 'checkbox',
    '#default_value' => 1,
  );

  // Check & display git status
  $cwd = getcwd();
  chdir($node->environment->root);
  $status = shell_exec('git status');
  chdir($cwd);

  $form['status'] = array(
    '#title' => t('Git Status'),
    '#type' => 'item',
    '#value' => '<pre>' . $status . '</pre>',
    '#weight' => -10,
  );
  return $form;
}
