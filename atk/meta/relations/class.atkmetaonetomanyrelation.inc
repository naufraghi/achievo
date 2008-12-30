<?php
/**
 * This file is part of the Achievo ATK distribution.
 * Detailed copyright and licensing information can be found
 * in the doc/COPYRIGHT and doc/LICENSE files which should be
 * included in the distribution.
 *
 * @package atk
 * @subpackage meta.relations
 *
 * @copyright (c) 2005 Peter C. Verhage <peter@achievo.org>
 * @license http://www.achievo.org/atk/licensing ATK Open Source License
 *
 * @version $Revision: 1.8 $
 * $Id: class.atkmetaonetomanyrelation.inc,v 1.8 2007/10/23 10:24:34 dennis Exp $
 */

/**
 * @internal Include the base class.
 */
userelation("atkonetomanyrelation");

/**
 * Implementation of one-to-many relationships for meta nodes.
 *
 * @author Peter C. Verhage <peter@achievo.org>
 * @package atk
 * @subpackage meta.relations
 */
class atkMetaOneToManyRelation extends atkOneToManyRelation
{
  var $m_accessor;
  var $m_options;

  /**
   * Default constructor.
   */
  function atkMetaOneToManyRelation($name, $accessor, $template, $options, $flags=0)
  {
    $this->atkOneToManyRelation($name, $accessor, "?", $flags);
    $this->m_accessor = $accessor;
    $this->m_options = $options;

    $this->setDescriptorTemplate($template);

    if (isset($options["filter"])) 
      $this->setDestinationFilter($options["filter"]);
  }

  /**
   * Create the instance of the destination.
   *
   * If succesful, the instance is stored in the m_destInstance member variable.
   *
   * @return boolean true if succesful, false if something went wrong.
   */
  function createDestination()
  {
    $policy = &$this->m_ownerInstance->getMetaPolicy();

    if ($this->m_destInstance == NULL)
    {
      $this->m_destination = $policy->findDestination($this->m_accessor, true);
      atkdebug("atkMetaOneToManyRelation: '{$this->m_accessor}' destination '{$this->m_destination}'");
      
      if ($this->m_destination == NULL) 
      {
        atkerror("No destination found for atkMetaOneToManyRelation with accessor '{$this->m_accessor}'!");
        return FALSE;
      }
      else if (!parent::createDestination()) 
      {
        atkerror("Cannot instantiate destination '{$this->m_destination}' for atkMetaOneToManyRelation with accessor '{$this->m_accessor}'!");        
        return FALSE;
      }  
    }
    
    if (is_array($this->m_refKey) && in_array('?', $this->m_refKey))
    {
      $this->m_refKey = $policy->findDestinationAttribute($this);
      atkdebug("atkMetaOneToManyRelation: '{$this->m_accessor}' reference key '{$this->m_refKey}'");
      if ($this->m_refKey != NULL)
        $this->m_refKey = array($this->m_refKey);
    }

    return is_object($this->m_destInstance) && $this->m_refKey != NULL;
  }
}
?>