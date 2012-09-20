<?php

/**
 * Class for the platform context.
 */
class provisionContext_project extends provisionContext {
  public $parent_key = 'server';

  static function option_documentation() {
    return array(
      '--project_name' => 'Project: The codename for this project.',
      '--code_path' => 'Project: The path to the project codebases.  (NOT the Drupal root)',
      '--git_url' => 'Project: The Git URL for this project.',
      '--base_url' => 'Project: the base URL that the dev/test/live subdomains will be attached to.',
      '--server' => 'Project: The server hosting this project.  (Default is @server_master)',
      '--install_profile' => 'Project: The desired installation profile for all sites.',
    );
  }

  function init_project() {
    $this->setProperty('project_name');
    $this->setProperty('code_path');
    $this->setProperty('install_profile');
    $this->setProperty('git_url');
    $this->setProperty('base_url');
  }
}