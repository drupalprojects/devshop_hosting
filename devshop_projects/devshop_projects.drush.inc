<?php
/**
 * @file devshop_projects.drush.inc
 * Keeping things organized.  All code that is really run via drush commands
 * will go here.
 */


/**
 * Implements drush_HOOK_pre_COMMAND()
 *
 * This runs for each tasks during the command
 *    drush @hostmaster hosting-tasks
 *
 * NOTE: This ONLY runs when being called from a hostmaster task.
 * This hook should ONLY be used to pass Options from a hostmaster task form to
 * the $task object, or if you don't need this functionality from the command
 * line.
 */
function drush_devshop_projects_pre_hosting_task() {

  // Verify Platform
  $task =& drush_get_context('HOSTING_TASK');

  // Verify Platform
  // For our platforms, we have to clone it if it has a git_remote
  // If it has a git branch, we should checkout as well.
  if ($task->ref->type == 'platform' && $task->task_type == 'verify' && !empty($task->ref->git_url)) {

    $platform = $task->ref;
    $root = $platform->publish_path;
    $git_remote = $platform->git_url;
    $git_branch = $platform->git_branch;

    // Check if a repo exists
    if (!is_dir($root) && !drush_shell_cd_and_exec($root, 'git status')){
      drush_log(dt("[DEVSHOP] No Repo found at !root.  Cloning from !git", array('!git' => $platform->git_url, '!root' => $root)));

      // Build the command string
      $command = "git clone $git_remote $root";
      if ($git_branch) {
        $command .= " --branch $git_branch";
      }
    }
    // If the platform has been verified and has a branch and git url
    else {
      $root = $platform->publish_path;
      $git_remote = $platform->git_url;
      $git_branch = $platform->git_branch;

      // Build the command string
      $command = "git checkout $git_branch";
    }

    // Execute
    if (!empty($command)){
       drush_log('[DEVSHOP] Running: ' . $command);

      // @TODO: Create a d()->server->shell_cd_and_exec() function
      //  server->shell_exec() uses escapeshellcmd(), so we cannot cd_and_exec!
      // So instead, this is the code from d()->server->shell_exec
      // d()->server->shell_exec($cmd);
      if (provision_is_local_host(d()->server->remote_host)) {
        return drush_shell_cd_and_exec($root, escapeshellcmd($command));
      }
      else {
        return drush_shell_cd_and_exec($root, 'ssh ' . drush_get_option('ssh-options', '-o PasswordAuthentication=no') . ' %s %s', $this->script_user . '@' . d()->server->remote_host, escapeshellcmd($command));
      }

       $output = drush_shell_exec_output();
       drush_log('Shell Output: ' . implode("\n", $output) , 'notice');
    }
  }
}

/**
 * Implementation of hook_post_hosting_TASK_TYPE_task
 *
 * If this is a new platform created by a project, we will create a site here.
 *
 * @TODO: This is used only for creating the new site when a "branch platform"
 * is created... we should save something in the task or platform node so we
 * don't have to use this logic.
 */
function devshop_projects_post_hosting_verify_task($task, $data) {

  // We only case about platforms.
  if ($task->ref->type != 'platform') {
    return;
  }

  // Get objects
  $nid = $task->ref->nid;
  $platform = node_load($nid);

  // If this platform isn't in a project, bail.
  if (empty($platform->project_nid)){
    drush_log('[DEVSHOP] No project found for this platform.', 'notice');
    return;
  }

  // Get the project
  $project = node_load($platform->project_nid);

  // If the project doesn't have an install profile chosen yet, bail.
  if (empty($project->install_profile)){
    drush_log('[DEVSHOP] No install profile found for this platform\'s project.', 'notice');
    return;
  }

  // If the project has a site already, bail.
  $sites = array_flip($project->project_objects['site']);
  $sites = hosting_get_sites_by_status($platform->nid, HOSTING_SITE_ENABLED);
  if (!empty($sites)){
    drush_log('[DEVSHOP] Platform already has a site.', 'notice');
    return;
  }

  // live. Let's create a site based off of this platform.
  drush_log('[DEVSHOP] Platform verified.  Creating your site.');


  $site_node = devshop_projects_create_site($project, $platform, $platform->environment);

  drush_log('[DEVSHOP] Site Created! Please wait for it to be installed. !link', array('!link' => l('node/' . $site_node->nid)));

}

/**
 * Implements hook_hosting_project_context_options()
 *
 * This transfers data from the node to thes aegir context object (the alias!)
 * For project entities.  This is where we find the branches and tags on the remote.
 */
function devshop_projects_hosting_project_context_options(&$task) {

  $task->context_options['server'] = '@server_master';
  $task->context_options['project_name'] = $task->ref->title;
  $task->context_options['install_profile'] = $task->ref->install_profile;
  $task->context_options['base_url'] = $task->ref->base_url;
  $task->context_options['code_path'] = trim($task->ref->code_path, " ");
  $task->context_options['git_url'] = $task->ref->git_url;

  $branches = getBranchesAndTags($task->ref->git_url);

  $task->ref->git_branches = $branches['branches'];
  $task->ref->git_tags = $branches['tags'];

  // Save the project node now that we have branches and tags.
  drush_log('[DEVSHOP] Trying to node save ' . $task->ref->title);
  node_save($task->ref);

  $task->context_options['git_branches'] = $branches['branches'];
  $task->context_options['git_tags'] = $branches['tags'];
}

/**
 * Helpfer for getting branches and tags from a git URL
 */
function getBranchesAndTags($git_url = NULL){
  if (is_null($git_url)){
    $git_url = drush_get_option('git_url');
  }
  $command = "git ls-remote {$git_url}";
  drush_shell_exec($command);
  $exec_output = drush_shell_exec_output();

  drush_log('[DEVSHOP] running '.$command, 'ok');
  drush_log('[DEVSHOP] ' . implode("\n", $exec_output), 'ok');

  // Check for Permission Denied
  // @TODO: Provide link to the Public key for the server.
  if ('Permission denied' == substr($exec_output[0], 0, 17)){
    drush_set_error(DRUSH_FRAMEWORK_ERROR, dt('[DEVSHOP] Error:'). implode("\n", $exec_output), 'error');
    return;
  }

  // If remote list is empty, something else went wrong.
  if (count($exec_output) == 1 && empty($exec_output[0])){
    drush_set_error(DRUSH_FRAMEWORK_ERROR, dt('Something went wrong.  Check the git URL and try again.'), 'error');
    return;
  }

  // Build tag and branch list
  $branches = array();
  $tags = array();

  foreach ($exec_output AS $line_string){

    // @TODO: Would love some regex love here
    // Example remote line:
    // 9fc5727c0823d8d3300ba5aae3328d5998033e45 refs/heads/3-00
    $line = trim(substr($line_string, 40));
    $ref = explode("/", $line);
    $branch = array_pop($ref);

    if ($ref[1] == 'heads') {
      $branches[] = $branch;
    } else if ($ref[1] == 'tags') {
      $tags[] = $branch;
    }
  }
  drush_log(dt('[DEVSHOP] Found !b branches and !t tags.', array('!b' => count($branches), '!t' => count($tags), )), 'ok');
  return array('branches' => $branches, 'tags' => $tags);
}

/**
 * Implements hook_hosting_site_context_options()
 *
 * This transfers data from the node to the aegir context object (the alias!)
 * For site entities.
 */
function devshop_projects_hosting_site_context_options(&$task) {
  $task->context_options['project'] = $task->ref->project;
  $task->context_options['nerd'] = 'vision';
}

/**
 * Implements hook_hosting_site_context_options()
 *
 * This transfers data from the node to the aegir context object (the alias!)
 * For site entities.
 */
function devshop_projects_hosting_platform_context_options(&$task) {
  if (!empty($task->ref->project)){
    $task->context_options['project'] = $task->ref->project;
    $task->properties['task properties'] = 'works';
    $task->ref->properties['task ref properties'] = 'works';

    d()->setProperty('setProperty', 'works');

    // @TODO: none of these work --^
  }
}
