<?php

        $name = @trim(stripslashes($_POST['name']));
        $tel = @trim(stripslashes($_POST['tel']));
        $email = @trim(stripslashes($_POST['email']));

        $email_from = $email;
        $email_to = 'info@easycool.io';

        $body = 'Name: ' . $name . "\n\n" . 'Telephone: ' . $tel . "\n\n" . 'Email: ' . $email;

        $success = @mail($email_to, $body, 'Name: ' . $name . "\n\n" . 'Telephone: ' . $tel . "\n\n" . 'Email: ' . $email);

    ?>

    <!DOCTYPE HTML>
    <html lang="en-US">

    <head>
        <script>
            alert("תודה, נחזור אלייך בהקדם האפשרי");
        </script>
        <meta HTTP-EQUIV="REFRESH" content="0; url=http://easycool.io/">
    </head>