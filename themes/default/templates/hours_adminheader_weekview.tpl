<form name="weekview" method="post" action="dispatch.php">
{$session_form}
<table border=0 cellpadding=0 cellspacing=0 width="100%" class="table">
  <tr>
    <td valign="top" align="left">
    <a href="{$prevweekurl}">{atktext previousweek}</a>
    {if $nextweekurl}&nbsp;&nbsp;<a href="{$nextweekurl}">{atktext nextweek}</a>{/if}
    &nbsp;&nbsp;<a href="{$thisweekurl}">{atktext thisweek}</a>
    &nbsp;&nbsp;<a href="{$dayviewurl}">{atktext dayview}</a>

    {if $lockurl}         &nbsp;&nbsp;<a href="{$lockurl}"><b>{atktext lock}</b>
    {else if $unlockurl}  &nbsp;&nbsp;<b>{atktext unlock}</b>
    {/if}
    
    </td><td valign="top" align="right">
    {$datejumper}
    &nbsp;<input type="submit" value="{atktext goto}">
    </td></tr></table></form>

    <b>{if $curuser.firstname && $curuser.lastname}{$curuser.firstname} {$curuser.lastname} - {/if}{atktext week} {$curdate}</b>
    {$lockicon}
    <br><br><br>