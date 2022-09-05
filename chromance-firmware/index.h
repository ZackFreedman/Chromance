const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>    
<html lang = \"en\">    
<head>    
    <title>This is a Bootstrap CDN example</title>    
    <meta name = \"viewport\" content = \"width = device-width, initial-scale=1\">
    <link rel = \"stylesheet\" href  = \"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\">
    
</head>
<body>
    <div class = \"container\">
        <h1 align = \"center\"> Leds Controller</h1>
        Click to set Chromance mode <a href="/mode/chromance">Chromance</a><br>
        Click to turn <a href="/mode/udp">UDP packets</a><br>
    </div>
    <div>
    <input type="color" id="color_picker" name="color_picker"
            value="#f6b73c">
    <label for="body">Color selector</label>
    </div>
    <div class="result"></div>
    <script src = \"https://ajax.googleapis.com/ajax/libs/jquery/1.12.0/jquery.min.js\"></script>    
    <script src = \"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\"></script>    
</body>
</html> 
<script>
    const selectElement = document.getElementById('color_picker');

    selectElement.addEventListener('change', (event) => {
        const result = document.querySelector('.result');
        result.textContent = `You changed color to ${event.target.value}`;
        
        var xhttp = new XMLHttpRequest();
        var targetHue = `/mode/set_hue?hue=${event.target.value.replace("#","0")}`
        console.log(targetHue)
        xhttp.open("GET", targetHue, true);
        xhttp.send();
    });
</script>
)=====";