const sections = ['Datapoints', 'Groups', 'Structs', 'Enums', 'Types'];
const sectionDescriptions = {
  Datapoints: 'Runtime values with group-relative IDs, types, access rules, and defaults.',
  Groups: 'Address spaces and persistence settings for datapoints.',
  Structs: 'Reusable structured types. Enter parameter definitions as JSON.',
  Enums: 'Named values backed by one of the supported integer types.',
  Types: 'Named aliases with optional compile-time range bounds.',
};
const fieldDefinitions = {
  Datapoints: [
    ['name', 'Name', 'text', true], ['group', 'Group', 'group', true], ['id', 'ID', 'number', true],
    ['type', 'Type', 'type', true], ['access', 'Access', 'access', true], ['version', 'Version', 'text', true],
    ['namespace', 'Namespace', 'text'], ['description', 'Description', 'textarea'],
    ['default', 'Default value (JSON)', 'json'], ['arraySize', 'Array size', 'number'],
    ['renamedFrom', 'Prior IDs (JSON array)', 'json'], ['migration', 'Migration callback', 'text'],
    ['allowUpgrade', 'Allow persistence upgrade', 'checkbox'],
  ],
  Groups: [
    ['name', 'Name', 'text', true], ['persistence', 'Persistence', 'persistence', true],
    ['baseId', 'Base ID', 'text', true], ['version', 'Version', 'text', true],
    ['description', 'Description', 'textarea'], ['renamedFrom', 'Prior IDs (JSON array)', 'json'],
    ['migration', 'Migration callback', 'text'], ['allowUpgrade', 'Allow persistence upgrade', 'checkbox'],
  ],
  Structs: [['name', 'Name', 'text', true], ['parameter', 'Parameters (JSON array)', 'json', true]],
  Enums: [
    ['name', 'Name', 'text', true], ['type', 'Underlying type', 'baseType', true],
    ['values', 'Values (JSON array)', 'json', true], ['autoId', 'Assign values automatically', 'checkbox'],
  ],
  Types: [
    ['name', 'Name', 'text', true], ['type', 'Underlying type', 'baseType', true],
    ['min', 'Minimum value (JSON)', 'json'], ['max', 'Maximum value (JSON)', 'json'],
  ],
};

let model = {};
let metadata = {};
let activeSection = 'Datapoints';
let editingIndex = null;
let itemModal;

function escapeText(value) {
  return value === undefined || value === null ? '' : String(value);
}

function showAlert(message, kind = 'danger') {
  const alert = document.createElement('div');
  alert.className = `alert alert-${kind} alert-dismissible fade show`;
  alert.setAttribute('role', 'alert');
  alert.textContent = message;
  const close = document.createElement('button');
  close.type = 'button';
  close.className = 'btn-close';
  close.dataset.bsDismiss = 'alert';
  close.setAttribute('aria-label', 'Close');
  alert.append(close);
  document.querySelector('#alert-region').replaceChildren(alert);
}

function updateStatus(validation) {
  const status = document.querySelector('#validation-status');
  status.textContent = validation.valid ? 'Valid' : 'Invalid';
  status.className = `badge ${validation.valid ? 'text-bg-success' : 'text-bg-danger'}`;
  if (!validation.valid && validation.error) {
    showAlert(validation.error);
  }
}

function renderNavigation() {
  const navigation = document.querySelector('#section-navigation');
  navigation.replaceChildren(...sections.map((section) => {
    const button = document.createElement('button');
    button.className = `nav-link ${section === activeSection ? 'active' : ''}`;
    button.type = 'button';
    button.textContent = `${section} (${model[section].length})`;
    button.addEventListener('click', () => {
      activeSection = section;
      render();
    });
    return button;
  }));
}

function visibleColumns(section) {
  const fields = fieldDefinitions[section];
  return fields.filter(([name, , type]) => type !== 'json' && type !== 'checkbox').slice(0, 5);
}

function renderTable() {
  const fields = visibleColumns(activeSection);
  const header = document.querySelector('#item-table-header');
  const headerRow = document.createElement('tr');
  fields.forEach(([, label]) => {
    const cell = document.createElement('th');
    cell.textContent = label;
    headerRow.append(cell);
  });
  header.replaceChildren(headerRow);

  const body = document.querySelector('#item-table-body');
  if (model[activeSection].length === 0) {
    const row = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = fields.length;
    cell.className = 'empty-state py-5';
    cell.textContent = `No ${activeSection.toLowerCase()} defined.`;
    row.append(cell);
    body.replaceChildren(row);
    return;
  }
  body.replaceChildren(...model[activeSection].map((item, index) => {
    const row = document.createElement('tr');
    row.addEventListener('click', () => openItemModal(index));
    fields.forEach(([name]) => {
      const cell = document.createElement('td');
      cell.textContent = escapeText(item[name]);
      row.append(cell);
    });
    return row;
  }));
}

function render() {
  document.querySelector('#section-title').textContent = activeSection;
  document.querySelector('#section-description').textContent = sectionDescriptions[activeSection];
  renderNavigation();
  renderTable();
}

function defaultItem(section) {
  if (section === 'Datapoints') {
    const group = metadata.groups[0] || '';
    return {name: '', group, id: metadata.nextIds[group] ?? 0, type: metadata.baseTypes[0], access: 'READ_WRITE', version: '1.0.0'};
  }
  if (section === 'Groups') return {name: '', persistence: 'None', baseId: '0x4000', version: '1.0.0'};
  if (section === 'Structs') return {name: '', parameter: []};
  if (section === 'Enums') return {name: '', type: 'uint32_t', values: [], autoId: true};
  return {name: '', type: 'uint32_t'};
}

function selectOptions(kind) {
  if (kind === 'group') return metadata.groups;
  if (kind === 'type') return metadata.types;
  if (kind === 'baseType') return metadata.baseTypes;
  if (kind === 'access') return metadata.accessModes;
  return metadata.persistenceModes;
}

function createInput(name, label, kind, required, value) {
  const wrapper = document.createElement('div');
  wrapper.className = 'mb-3';
  const inputId = `field-${name}`;
  const labelElement = document.createElement('label');
  labelElement.className = 'form-label';
  labelElement.htmlFor = inputId;
  labelElement.textContent = label;
  wrapper.append(labelElement);

  let input;
  if (['group', 'type', 'baseType', 'access', 'persistence'].includes(kind)) {
    input = document.createElement('select');
    selectOptions(kind).forEach((option) => {
      const optionElement = document.createElement('option');
      optionElement.value = option;
      optionElement.textContent = option;
      input.append(optionElement);
    });
  } else if (kind === 'textarea' || kind === 'json') {
    input = document.createElement('textarea');
    input.rows = kind === 'json' ? 4 : 2;
    input.value = kind === 'json' && value !== undefined ? JSON.stringify(value, null, 2) : escapeText(value);
    if (kind === 'json') input.classList.add('font-monospace');
  } else {
    input = document.createElement('input');
    input.type = kind === 'checkbox' ? 'checkbox' : kind;
    if (kind === 'checkbox') {
      input.checked = Boolean(value);
      wrapper.className = 'form-check mb-3';
      labelElement.className = 'form-check-label';
      wrapper.replaceChildren(input, labelElement);
    } else {
      input.value = escapeText(value);
    }
  }
  input.id = inputId;
  input.name = name;
  input.required = Boolean(required);
  input.classList.add(kind === 'checkbox' ? 'form-check-input' : 'form-control');
  if (input.tagName === 'SELECT') {
    input.className = 'form-select';
    input.value = escapeText(value);
  }
  if (input.tagName !== 'INPUT' || kind !== 'checkbox') wrapper.append(input);
  return wrapper;
}

function openItemModal(index) {
  editingIndex = index;
  const item = index === null ? defaultItem(activeSection) : model[activeSection][index];
  document.querySelector('#item-modal-title').textContent = `${index === null ? 'Add' : 'Edit'} ${activeSection.slice(0, -1)}`;
  const body = document.querySelector('#item-modal-body');
  body.replaceChildren(...fieldDefinitions[activeSection].map(([name, label, kind, required]) =>
    createInput(name, label, kind, required, item[name])));
  document.querySelector('#delete-button').hidden = index === null;
  itemModal.show();
}

function readItemForm() {
  const item = {};
  for (const [name, , kind, required] of fieldDefinitions[activeSection]) {
    const input = document.querySelector(`#field-${name}`);
    if (kind === 'checkbox') {
      if (input.checked) item[name] = true;
      continue;
    }
    const text = input.value.trim();
    if (!text && !required) continue;
    if (kind === 'json') {
      try {
        item[name] = JSON.parse(text);
      } catch (error) {
        throw new Error(`${name} must contain valid JSON.`);
      }
    } else if (kind === 'number') {
      item[name] = Number(text);
    } else {
      item[name] = text;
    }
  }
  return item;
}

async function requestValidation() {
  const response = await fetch('/api/validate', {
    method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({model}),
  });
  const validation = await response.json();
  updateStatus(validation);
  return validation;
}

async function loadModel() {
  const response = await fetch('/api/model');
  if (!response.ok) throw new Error('Unable to load the model.');
  const data = await response.json();
  model = data.model;
  metadata = data.metadata;
  updateStatus(data.validation);
  render();
}

document.addEventListener('DOMContentLoaded', async () => {
  itemModal = new bootstrap.Modal('#item-modal');
  document.querySelector('#add-button').addEventListener('click', () => openItemModal(null));
  document.querySelector('#item-form').addEventListener('submit', async (event) => {
    event.preventDefault();
    try {
      const item = readItemForm();
      if (editingIndex === null) model[activeSection].push(item);
      else model[activeSection][editingIndex] = item;
      itemModal.hide();
      render();
      await requestValidation();
    } catch (error) {
      showAlert(error.message);
    }
  });
  document.querySelector('#delete-button').addEventListener('click', async () => {
    model[activeSection].splice(editingIndex, 1);
    itemModal.hide();
    render();
    await requestValidation();
  });
  document.querySelector('#validate-button').addEventListener('click', requestValidation);
  document.querySelector('#save-button').addEventListener('click', async () => {
    const response = await fetch('/api/model', {
      method: 'PUT', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({model}),
    });
    const result = await response.json();
    updateStatus(result);
    if (!response.ok) return;
    metadata = result.metadata;
    showAlert('Model saved.', 'success');
    render();
  });
  try {
    await loadModel();
  } catch (error) {
    showAlert(error.message);
  }
});
