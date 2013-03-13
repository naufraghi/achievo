<?php
/*
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All rights reserved.
 *
 * $Id:$
 * Author: Matteo Bertini <naufraghi@develer.com>
 */

/*
SELECT userid, firstname, lastname, activitydate, time_contract, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, firstname, lastname, activitydate, time_contract,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent > 5*60) AS tickets
            FROM (
                SELECT person.userid, person.firstname, person.lastname,
                    usercontract.uc_hours/5 as time_contract, project.name AS project_name,
                    phase.name AS phase_name, hours.activitydate, sum(hours.time) AS time_spent,
                    (project.name = '__dipendenti' AND phase.name = 'malattia') AS malattia,
                    (project.name = '__dipendenti' AND phase.name = 'ferie') AS ferie,
                    (project.name LIKE 'd20_%') AS d20
                FROM hours
                    JOIN person ON hours.userid = person.id
                    JOIN phase ON hours.phaseid = phase.id
                    JOIN activity ON hours.activityid = activity.id
                    JOIN project ON phase.projectid = project.id
                    JOIN usercontract ON usercontract.userid = person.id
                    WHERE activitydate > '2008-08-05'
                GROUP BY person.firstname, person.lastname, person.userid, usercontract.uc_hours,
                         hours.activitydate, malattia, ferie, d20
            ) AS lavoro
        GROUP BY userid, firstname, lastname, activitydate, time_contract, malattia, ferie, d20
    ) AS parts
GROUP BY userid, firstname, lastname, activitydate, time_contract
*/

useattrib("atkdateattribute");
atkImport("modules.scheduler.scheduler_holidays");
atkImport("modules.reports.weekreport");

function str_number_format($var, $digits)
{
    if (is_numeric($var))
        return number_format($var, $digits, ",", ".");
    else
        return $var;
}

//Supporto per la colorazione alternata delle righe
$_flipflop = true;
function flipflop($flip=true, $flop=false)
{
    global $_flipflop;
    $_flipflop = !$_flipflop;
    if ($_flipflop)
        return $flip;
    else
        return $flop;
}

function get_days_array($date_start, $date_end)
{
    $start_date = date_create($date_start);
    $stop_date = date_create($date_end);
    $curr_date = date_create($date_start);
    $days = array();
    while ($curr_date < $stop_date)
    {
        array_push($days, date_create($curr_date->format(DATE_ATOM)));
        $curr_date->modify("+1 day");
    }
    return $days;
}

function float2str($float, $show_zero=true)
{
    return ($float or $show_zero) ? str_number_format($float, 2) : "";
}

function format_date($date)
{
    return $date->format("Y-m-d");
}

class timebalance extends atkNode
{
    var $m_user = "";

    function timebalance()
    {
        $this->atkNode("timebalance");
        $this->overtime_node = &atkGetNode("timereg.overtime_balance");
        $this->holidays_node = &atkGetNode("scheduler.scheduler_holidays");
        $this->weekreport_node = &atkGetNode("reports.weekreport");
        $this->m_user = $this->processGlobalVar("userid");
        atkdebug("DEBUG: m_user = ".$this->m_user);
        if($this->m_user=="") $this->m_user = $this->getInitialUser();
    }

    function getInitialUser() {
        global $g_user;
        return $g_user["id"];
    }

    function processGlobalVar($name, $value="")
    {
      $sessionManager = &atkGetSessionManager();
      $session = &$sessionManager->getSession();
      $sessionvarid = "timebalance_$name";
      if (empty($value) && isset($_REQUEST[$name])) $value = $_REQUEST[$name];
      if ($value!=="") $session[$sessionvarid] = $value;
      return $session[$sessionvarid];
    }

    function get_timesummary()
    {
        $userid = $this->m_user;
        $db = &atkGetDb();

        /* Accetta una data in ingresso oppure imposta l'intervallo all'ultimo mese appena concluso*/
        $last_month_start = mktime(0, 0, 0, date("m")-1, 1, date("Y"));
        $last_month_end = mktime(0, 0, 0, date("m"), 1, date("Y"));
        $date_start = $this->m_postvars['date_start'];
        $date_end = $this->m_postvars['date_end'];
        if ($date_start == "")
            $date_start = date("Y-m-d", $last_month_start);
        else
            $date_start = sprintf("%04d",$date_start["year"])."-".sprintf("%02d",$date_start["month"])."-".sprintf("%02d",$date_start["day"]);
        if ($date_end == "")
            $date_end = date("Y-m-d", $last_month_end);
        else
            $date_end = sprintf("%04d",$date_end["year"])."-".sprintf("%02d",$date_end["month"])."-".sprintf("%02d",$date_end["day"]);

        if ( $userid != 'all' )
            $single_user = "AND hours.userid ='$userid'";
        else
            $single_user = "";

        /* Rimossa la join con usercontract dato che era la causa del problema
        e i dati presenti nella tabella non erano utilizzati da nessuna parte
        nella query. */
        $ore_registrate = "
                SELECT person.id, person.userid, person.firstname, person.lastname,
                    hours.activitydate, hours.time/60 AS time_spent,
                    (project.name = '__dipendenti' AND phase.name = 'malattia') AS malattia,
                    (project.name = '__dipendenti' AND phase.name = 'ferie') AS ferie,
                    (project.name = '__dipendenti' AND LOWER(phase.name) LIKE 'permessi%') AS permessi,
                    (project.name LIKE 'd20_%') AS d20
                FROM hours
                    JOIN person ON hours.userid = person.id
                    JOIN phase ON hours.phaseid = phase.id
                    JOIN activity ON hours.activityid = activity.id
                    JOIN project ON phase.projectid = project.id
                WHERE activitydate >= '$date_start'
                    AND activitydate < '$date_end'
                    $single_user
        ";

        $select = "
SELECT id, userid, firstname, lastname, activitydate, SUM(time_worked) as time_worked,
       SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_permessi) as time_permessi,
       SUM(time_d20) as time_d20, (SUM(time_worked) >= 5) as tickets
    FROM (
        SELECT id, userid, firstname, lastname, activitydate,
            (NOT (malattia OR ferie OR permessi OR d20)) * time_spent AS time_worked,
            malattia * time_spent AS time_malattia,
            ferie * time_spent AS time_ferie,
            permessi * time_spent AS time_permessi,
            d20 * time_spent AS time_d20
        FROM (
            $ore_registrate
        ) AS lavoro
    ) AS parts
GROUP BY id, userid, firstname, lastname, activitydate;
";
        if ( $userid != 'all' ) {
            $rows = $db->getRows($select);
        } else {
            $rows = array();
        }
        return array("date_start" => $date_start,
                     "date_end" => $date_end,
                     "rows" => $rows);
    }

    function get_hours_table($timesummary)
    {
        $userid = $this->m_user;
        $this->addStyle("style.css");
        $content = '<form action="dispatch.php" method="get" name="entryform">';
        $content .= session_form();

        $title = "TimeBalance: ".$timesummary["date_start"]." - ".$timesummary["date_end"]." (escluso)";

        $startdateatt = new atkDateAttribute("date_start", "F d Y", "d F Y",0 , 0, AF_OBLIGATORY);
        $enddateatt = new atkDateAttribute("date_end", "F d Y", "d F Y", 0, 0, AF_OBLIGATORY);

        $_startdate = array("date_start" => $startdateatt->dateArray($timesummary["date_start"]));
        $_enddate = array("date_end" => $enddateatt->dateArray($timesummary["date_end"]));

        $content .= "Da ".$startdateatt->edit($_startdate)." a ".$enddateatt->edit($_enddate)."<br>\n";
        $content .= '<input type="button" onclick="ATK.DateAttribute.modifyValue(\'date_start\', {month: \'+1\'}); ATK.DateAttribute.modifyValue(\'date_end\', {month: \'+1\'})" value="Mese+" />';
        $content .= '<input type="button" onclick="ATK.DateAttribute.modifyValue(\'date_start\', {month: \'-1\'}); ATK.DateAttribute.modifyValue(\'date_end\', {month: \'-1\'})" value="Mese-" />';
        $weekreport = &atkGetNode("reports.weekreport");
        $content .= 'User: <SELECT name="userid">'.$weekreport->get_employees($userid, $key="userid").'</SELECT>';
        $content .= '<input type="submit" value="'.atktext("refresh").'">';
        $content .= "<h2>$title</h2>\n";

        // Prepara dati in struttura 3d
        $daykeys = array_map("format_date", get_days_array($timesummary["date_start"], $timesummary["date_end"]));
        // FIXME: rotto se userid == all
        $workdays = $this->overtime_node->enumWorkingDays($timesummary["date_start"], $timesummary["date_end"], $userid, 'Y-m-d');
        atkdebug("workdays: ".var_export($workdays, 1));
        $alldays = array();
        foreach ($daykeys as $day) {
            $alldays[$day] = array('time_contract' => floatval($workdays[$day]),
                                   'time_worked' => 0.0,
                                   'time_malattia' => 0.0,
                                   'time_ferie' => 0.0,
                                   'time_permessi' => 0.0,
                                   'time_d20' => 0.0,
                                   'tickets' => 0,
                                   'lavoro' => false);
        }
        $time_balance = array();
        $userinfo = $this->weekreport_node->get_employee($userid);
        $time_balance[$userinfo['userid']] = array('id' => $userinfo['id'],
                                                   'fullname' => $userinfo['firstname']." ".$userinfo['lastname'],
                                                   'activity' => $alldays,
                                                   'tot_contract' => 0.0,
                                                   'tot_registro' => 0.0,
                                                   'tot_worked' => 0.0,
                                                   'tot_malattia' => 0.0,
                                                   'tot_ferie' => 0.0,
                                                   'tot_permessi' => 0.0,
                                                   'tot_d20' => 0.0,
                                                   'tot_tickets' => 0,
                                                  );
        foreach ($timesummary['rows'] as $key => $r)
        {
            $activitydate = array('time_worked' => floatval($r['time_worked']),
                                  'time_malattia' => floatval($r['time_malattia']),
                                  'time_ferie' => floatval($r['time_ferie']),
                                  'time_permessi' => floatval($r['time_permessi']),
                                  'time_d20' => floatval($r['time_d20']),
                                  'tickets' => intval($r['tickets']),
                                  'lavoro' => true);
            $activitydate = array_merge($time_balance[$r['userid']]['activity'][$r['activitydate']], $activitydate);

            $time_balance[$r['userid']]['activity'][$r['activitydate']] = $activitydate;
            $time_balance[$r['userid']]['tot_worked'] += $activitydate['time_worked'];
            $time_balance[$r['userid']]['tot_malattia'] += $activitydate['time_malattia'];
            $time_balance[$r['userid']]['tot_ferie'] += $activitydate['time_ferie'];
            $time_balance[$r['userid']]['tot_permessi'] += $activitydate['time_permessi'];
            $time_balance[$r['userid']]['tot_d20'] += $activitydate['time_d20'];
            $time_balance[$r['userid']]['tot_tickets'] += $activitydate['tickets'];

            $time_balance[$r['userid']]['activity'][$r['activitydate']]['totale_parziale_worked'] = $time_balance[$r['userid']]['tot_worked'];
            atkdebug("TimeBalance loop: ".var_export($r, 1));
        }
        atkdebug("TimeBalance: ".var_export($time_balance, 1));
        return array("content" => $content, "time_balance" => $time_balance, "title" => $title);
    }

    function action_report()
    {
        $timesummary = $this->get_timesummary();
        $hours_table = $this->get_hours_table($timesummary);
        $content = $hours_table['content'];
        // Tabelle banca ore
        $banca_head = "<tr><th>User</th><th>Nome</th><th>Giorno</th><th>Ore previste</th><th>Ore registro</th>
                           <th>Ore lavorate</th><th>Malattia</th><th>Ferie</th><th>Permessi</th><th>Develer20</th><th>Tickets</th>
                           </tr>\n";
        foreach ($hours_table['time_balance'] as $userid => &$userdata)
        {
            $content .= "<h3>$userid - ".$userdata['fullname']."</h3>\n";
            $content .= "<table border='1'>\n";
            $content .= $banca_head;

            /*
            Affinche' il conteggio degli straordinari funzioni correttamente
            e' necessario che:
             * il giorno in cui si calcola il credito/debito di ore all'inizio
             del range selezionato sia l'ultimo del mese precedente.
             * il giorno in cui si calcola il credito/debito di ore alla fine
             del bilancio sia l'ultimo del mese in corso.

            Pertanto i valori di date_start e date_end sono modificati per
            rispecchiare queste specifiche.

            NOTE: queste modifica ha come requisito l'eliminazione dei check
            inseriti da lmancini nella revisione r28891. Questo potrebbe
            portare a malfunzionamenti. Ergo, prima di inserire le modifiche
            sull'achievo in produzione sarebbe raccomandabile il testarlo. A
            fondo.
            */
            $tmp_d = date_create($timesummary["date_start"]);
            $tmp_d->modify("-1 day");
            $tmp_d = format_date($tmp_d);
            $userdata['date_start']['balance'] = $this->overtime_node->getBalance($tmp_d, $userdata['id']);

            $tmp_d = date_create($timesummary["date_end"]);
            $tmp_d->modify("-1 day");
            $tmp_d = format_date($tmp_d);
            $userdata['date_end']['balance'] = $this->overtime_node->getBalance($tmp_d, $userdata['id']);

            $banca_ore = $userdata['date_start']['balance']['balance'] + $userdata['tot_worked'];

            foreach ($userdata['activity'] as $date => &$activity)
            {
                if ( $this->holidays_node->isHoliday($date) ) {
                    $bgcolor = "#CCBBFF";
                } else {
                    $bgcolor = flipflop("#9999FF", "#BBBBFF");
                }
                $abstime = strtotime($date);

                $content .= "<tr style='background: $bgcolor'>";
                $content .= "<td>$userid</td>";
                $content .= "<td>".$userdata['fullname']."</td>";
                if ( $this->holidays_node->isHoliday($date) ) {
                    $content .= "<td><b><i>".strftime("%Y-%m-%d %a", $abstime)."</i></b></td>";
                } elseif ( !($activity['time_contract'] > 0) ) {
                    $content .= "<td><strike>".strftime("%Y-%m-%d %a", $abstime)."</strike></td>";
                } else {
                    $content .= "<td>".strftime("%Y-%m-%d %a", $abstime)."</td>";
                }
                if ( $activity['lavoro'] or ($activity['time_contract'] > 0) ) {
                    $userdata['tot_contract'] += $activity['time_contract'];
                    $content .= "<td align='right' title='".float2str($userdata['tot_contract'])."'>".float2str($activity['time_contract'])."</td>";
                    if ($banca_ore > 0) {
                        $ore_registro = $activity['time_contract'] - $activity['time_malattia'] - $activity['time_permessi'];
                        $ore_registro = min($ore_registro, $banca_ore);
                    } else {
                        $ore_registro = 0.0;
                    }
                    $banca_ore -= $ore_registro;
                    $content .= "<td align='right'>".float2str($ore_registro)."</td>";
                    $activity['time_registro'] = $ore_registro;
                    $userdata['tot_registro'] += $ore_registro;
                } else {
                    $content .= "<td align='right'>&nbsp</td>"; // time_contract
                    $content .= "<td align='right'>&nbsp</td>"; // ore_registro
                }
                if ( $activity['lavoro'] or ($activity['time_contract'] > 0) )
                {
                    $content .= "<td align='right' title='".float2str($activity['totale_parziale_worked'])."'>".float2str($activity['time_worked'])."</td>";
                    $content .= "<td align='right'>".float2str($activity['time_malattia'])."</td>";
                    $content .= "<td align='right'>".float2str($activity['time_ferie'])."</td>";
                    $content .= "<td align='right'>".float2str($activity['time_permessi'])."</td>";
                    $content .= "<td align='right'>".float2str($activity['time_d20'])."</td>";
                    $content .= "<td align='right'>".$activity['tickets']."</td>";
                }
                else
                {
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                }
                $content .= "</tr>\n";
            } // fine loop su tutti i giorni selezionati
            $content .= $banca_head;
            $content .= "<tr><td>$userid</td><td>".$userdata['fullname']."</td><th>Totale</th>";
            $content .= "<th align='right'>".float2str($userdata['tot_contract'])."</th>";
            $content .= "<th align='right'>".float2str($userdata['tot_registro'])."</th>";
            $content .= "<th align='right' title='".float2str($userdata['tot_worked'] - $userdata['tot_registro'])."'>";
                $content .= float2str($userdata['tot_worked'])."</th>";
            $content .= "<th align='right'>".float2str($userdata['tot_malattia'])."</th>";
            $content .= "<th align='right'>".float2str($userdata['tot_ferie'])."</th>";
            $content .= "<th align='right'>".float2str($userdata['tot_permessi'])."</th>";
            $content .= "<th align='right' title='".float2str($userdata['tot_d20'])."'>";
                $content .= float2str(min($userdata['tot_d20'], $userdata['tot_worked']*0.2))."</th>";
            $content .= "<th align='right'>".$userdata['tot_tickets']."</th>";
            $content .= "</tr>\n";
            $content .= "<tr><td colspan='11'>$title<dl>\n";
            $content .= "<b>$userid:</b> ".$userdata['fullname']."<br>";
            $content .= "<b>Banca ore al ".$timesummary["date_start"].":</b> ".$this->overtime_node->getOvertimeLink($userdata['date_start'])."<br>";
            /*
            Aggiunto link per permettere di modificare il quantitativo di ore
            presenti in banca ore dopo aver stabilito con il dipendente quante
            di queste devono essere pagate come straordinari e quante messe da
            parte.
            */
            $content .= "<b>Banca ore al ".$timesummary["date_end"].":</b> ".$this->overtime_node->getOvertimeLink($userdata['date_end'])."<br>";
            $content .= "<b>Ore Previste:</b> ".float2str($userdata['tot_contract'])."<br>";
            $content .= "<b>Ore Registro:</b> ".float2str($userdata['tot_registro'])."<br>";
            $content .= "<b>Ore Lavorate:</b> <a title='".float2str($userdata['tot_worked'] - $userdata['tot_registro'])."'>";
                $content .= float2str($userdata['tot_worked'])."</a><br>";
            $content .= "<b>Malattia:</b> ".float2str($userdata['tot_malattia'])."<br>";
            $content .= "<b>Ferie:</b> ".float2str($userdata['tot_ferie'])."<br>";
            $content .= "<b>Permessi:</b> ".float2str($userdata['tot_permessi'])."<br>";
            $content .= "<b>Develer20:</b> <a title='".float2str($userdata['tot_d20'])."'>";
                $content .= float2str(min($userdata['tot_d20'], $userdata['tot_worked']*0.2))."</a><br>";
            $content .= "<b>Tickets:</b> ".$userdata['tot_tickets']."<br>";

            $content .= "</dl></td></tr>\n";
            $content .= "</table>\n";
        }

        $content .= "<p>Pagina creata il ".date_create()->format("Y-m-d")."</p>\n";
        $content .= '<input type="submit" value="'.atktext("refresh").'">';
        $content .= '<input type="hidden" name="atknodetype" value="'.$this->atkNodeType().'">';
        $content .= '<input type="hidden" name="atkaction" value="report">';
        $content .= '</form>';

        $content .= '<form action="dispatch.php" method="get" name="entryform">';
        $content .= '<input type="submit" value="'.atktext("Esport CSV").'">';
        $content .= '<input type="hidden" name="atknodetype" value="'.$this->atkNodeType().'">';
        $content .= '<input type="hidden" name="atkaction" value="export">';
        $content .= '</form>';

        if (!$export) {
            $page = &$this->getPage();
            $ui = &$this->getUi();
            $page->addContent($ui->renderBox(array("title"=>$hours_table['title'], "content"=>$content)));
        } else {
            return $hours_table;
        }

    }
    function action_export()
    {
        setlocale(LC_ALL, 'it_IT');
        $exporter = &atknew("atk.utils.atkfileexport");
        $timesummary = $this->get_timesummary();
        $hours_table = $this->get_hours_table($timesummary);
        $report_time = strtotime($timesummary["date_start"]);
        $table = "DEVELER SRL Registro presenze Anno ".strftime("%Y", $report_time)." mese di ".strftime("%B", $report_time)."\n";
        foreach ($hours_table['time_balance'] as $userid => $userdata)
        {
            $table .= ";Totali";
            foreach ($userdata['activity'] as $date => $activity)
            {
                $abstime = strtotime($date);
                $table .= strftime(";%a %d", $abstime);
            }
            $table .= "\n";
            $table .= $userdata['fullname'].";\n";
        }
        $exporter->export($table, "prova", "csv");
    }
}
?>
