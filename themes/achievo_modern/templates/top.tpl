<div id="banner">
  <img src="{$themedir}images/logo.jpg" alt="Logo Achievo" />
  <div id="topLinks">
    <span id="top-center">{foreach  from=$centerpiece_links item=link}{$link}&nbsp;&nbsp;|&nbsp;&nbsp;{/foreach}</span>
    <span id="top-logout"><a href="index.php?atklogout=1" target="{$logouttarget}">{atktext logout}</a></span>&nbsp;&nbsp;|&nbsp;&nbsp;{atktext search}&nbsp;
    <span id="top-search">{$searchpiece}</span>
  </div>
  <div id="loginBox">
    {atktext logged_in_as}: [{$user}] {$username}
  </div>
</div>