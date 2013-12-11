$(function() {
  //  var pl = "webm/root.json";
  var pl = "webm/late.json";

  function getJson(url, cb) {
    console.log("getJson(" + url + ")");
    $.ajax({
      url: url,
      context: this,
      dataType: 'json',
      global: false,
      success: cb
    });
  }

  function getBin(url, cb) {
    var xhr = new XMLHttpRequest();
    var now = new Date().getTime();
    xhr.responseType = "arraybuffer";
    xhr.onreadystatechange = function() {
      if (xhr.readyState == 4) {
        var elapsed = new Date().getTime() - now;
        if (xhr.status == 200 || xhr.status == 0) cb(new Uint8Array(xhr.response), elapsed);
        else throw "Can't get " + url;
      }
    }

    xhr.ontimeout = function() {
      throw "Timeout for " + url;
    }

    console.log("GET " + url);
    xhr.open('GET', url, true);
    xhr.send();
  }

  function loadChunks(sb, manifest, pos) {
    if (pos < manifest.length) {
      getBin('webm/' + manifest[pos].url, function(d) {
        sb.appendBuffer(d);
        loadChunks(sb, manifest, pos + 1);
      });
    }
  }

  getJson(pl, function(manifest) {
    var $player = $('#player');
    var player = $player.get(0);

    var ms = new MediaSource();

    ms.addEventListener('sourceopen', function(e) {
      console.log("Loading chunks");
      var sb = ms.addSourceBuffer('audio/webm; codecs="vorbis"');
      loadChunks(sb, manifest, 0);
    },
    false);

    player.src = window.URL.createObjectURL(ms);

  });

});
