function OggPipe() {}

OggPipe.prototype = {
  get: function(url, good, bad) {
    var xhr = new XMLHttpRequest();
    var now = new Date().getTime();
    xhr.responseType = "arraybuffer";
    xhr.onreadystatechange = function() {
      console.log("readyState: " + xhr.readyState);
      if (xhr.readyState == 4) {
        var elapsed = new Date().getTime() - now;
        if (xhr.status == 200 || xhr.status == 0) good(xhr, elapsed);
        else bad(xhr, elapsed);
      }
    }

    xhr.ontimeout = function() {
      console.log("Timeout");
      bad(xhr, elapsed)
    }

    console.log("GET " + url);
    xhr.open('GET', url, true);
    xhr.send();
  }
}
