document.addEventListener('DOMContentLoaded', function (event) {
  let websocket;

  const networkTable = document.querySelector('#networkTable');
  const tableBody = networkTable.querySelector('tbody');
  const tableCollapse = document.querySelector('#credentialForm');
  const refreshButton = document.querySelector('#refreshNetworks');
  const template = document.querySelector('#networkTableRow');
  let closing = false;

  function initWebSocket(restart) {
    websocket = new WebSocket('ws://' + window.location.hostname + '/ws');

    websocket.onopen = function () {
      console.log('WebSocket connection opened');
      websocket.send('scan');
    };

    websocket.onclose = function () {
      console.log('WebSocket connection closed');
      if (!restart && !closing) {
        alert(
          'Web socket disconnected. Please click refresh to restart auto network scanning.'
        );
      }
      refreshButton.disabled = false;
    };

    const ssidInput = document.querySelector('#ssid');

    websocket.onmessage = function (event) {
      console.log('Received data: ' + event.data);
      let networks = JSON.parse(event.data);
      tableBody.replaceChildren();
      for (let key in networks) {
        const strength = networks[key];
        let level = 0;

        switch (true) {
          case strength > -30:
            level = 4;
            break;
          case strength > -67:
            level = 3;
            break;
          case strength > -70:
            level = 2;
            break;
          case strength > -80:
            level = 1;
            break;
        }

        const elm = template.content.cloneNode(true).firstElementChild;

        for (let i = 4 - level; i > 0; i--) {
          elm.querySelector('.wifi-' + (4 - i + 1)).classList.add('opacity-30');
        }
        elm.querySelector('.networkName').textContent = key;
        tableBody.appendChild(elm);
      }

      document.querySelectorAll('.networkSelect').forEach((elm) => {
        elm.addEventListener('click', (evt) => {
          ssidInput.value = elm.previousElementSibling.textContent;
        });
      });

      tableCollapse.style.maxHeight = tableCollapse.scrollHeight + 'px';
      refreshButton.disabled = false;

      return false;
    };
  }

  const loadingNetwork =
    document.querySelector('#loadingNetwork');

  refreshButton.addEventListener('click', (evt) => {
    initWebSocket(false);
    evt.target.disabled = true;
    tableBody.replaceChildren();
    tableBody.appendChild(loadingNetwork.content.cloneNode(true).firstElementChild);
  });

  refreshButton.dispatchEvent(new Event('click'));

  let coll = document.querySelectorAll('.collapse-toggle');

  coll.forEach((elm) => {
    elm.addEventListener('click', toggleCollapse);
    setTimeout(() => {
      elm.dispatchEvent(new Event('click'));
    }, 250);
  });

  function toggleCollapse() {
    this.classList.toggle('active');
    let target = document.querySelector(this.dataset.toggle);
    if (target.style.maxHeight) {
      target.style.overflow = 'hidden';
      target.style.maxHeight = null;
    } else {
      target.style.overflow = 'visible';
      target.style.maxHeight = target.scrollHeight + 'px';
    }

    let icon = this.querySelector('.collapse-icon');
    if (icon) {
      icon.innerHTML =
        icon.innerHTML === String.fromCharCode(9650) ? '&#x25BC;' : '&#x25B2;';
    }
  }

  function updateBrightness(elm, percentage) {
    elm.style.filter = 'brightness(' + percentage + '%)';
  }

  document.querySelectorAll('.slideContainer').forEach((elm) => {
    elm.children[0].addEventListener('input', (evt) => {
      evt.target.nextElementSibling.value = evt.target.value;
      if (evt.target.classList.contains('brightness-slider')) {
        updateBrightness(
          elm.parentElement.querySelector('.brightness-box'),
          (evt.target.value / 255) * 100
        );
      }
    });

    elm.children[1].addEventListener('input', (evt) => {
      evt.target.previousElementSibling.value = evt.target.value;
      if (evt.target.value.length > 1) {
        evt.target.value = Math.round(evt.target.value.replace(/\D/g, ''));
        if (parseInt(evt.target.value) < parseInt(evt.target.min)) {
          evt.target.value = evt.target.min;
        }
        if (parseInt(evt.target.value) > parseInt(evt.target.max)) {
          evt.target.value = evt.target.max;
        }
      }
      if (evt.target.classList.contains('brightness-number')) {
          updateBrightness(
            elm.parentElement.querySelector('.brightness-box'),
            (evt.target.value / 255) * 100
          );
        }
    });
  });

  function formInputReadonly(form, readonly) {
    form.querySelectorAll('input').forEach((input) => {
      input.readOnly = readonly;
    });
  }

  document
    .querySelector('#configForm')
    .addEventListener('submit', async (evt) => {
      evt.preventDefault();
      formInputReadonly(evt.target, true);
      let formData = new FormData(evt.target);
      let object = {};
      formData.forEach((value, key) => object[key] = value);
      let json = JSON.stringify(object);
      console.log(json);
      try {
        alert("Form submitted. Validation may take up to max 1 minute.");
        const response = await fetch('/submitForm', {
          method: 'POST',
          mode: 'cors',
          body: json,
          headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
          },
        });
        const data = await response.json();
        console.log(data);
        alert(data? 'Configuration is saved and web server will be closed.': 'Invalid configuration. Please check again.');
        if (data) {
          closing = true;
          window.location.href = "about:home";
        }
      } catch (e) {
        alert("Error:" + e);
        console.error("Error:", e);
      } finally {
        formInputReadonly(evt.target, false);
      }
    });
});
