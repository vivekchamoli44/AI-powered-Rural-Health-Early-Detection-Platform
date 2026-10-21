document.addEventListener('DOMContentLoaded', function () {
    const openBtn = document.getElementById('open-assess-modal');
    const modal = document.getElementById('assess-modal');
    const form = document.getElementById('assessment-form');
    // Open assessment modal
    if (openBtn && modal) {
        openBtn.addEventListener('click', function () {
            if (form) form.reset();
            modal.classList.add('active');
        });
    }
    // Submit assessment form
    if (form) {
        form.addEventListener('submit', handleAssessmentSubmit);
    }
});
// 1. Submit Health Assessment
async function handleAssessmentSubmit(e) {
    e.preventDefault();
    const submitBtn = document.getElementById('submit-assess-btn');
    submitBtn.disabled = true;
    submitBtn.textContent = 'Running AI Diagnosis...';
    // Get current logged in patient info
    let user = null;
    try {
        user = JSON.parse(localStorage.getItem('auth_user'));
    } catch (err) {}
    // Collect selected symptom checkboxes
    const symptoms = [];
    document.querySelectorAll('input[name="symptom_item"]:checked').forEach(function (cb) {
        symptoms.push(cb.value);
    });
    const duration = document.getElementById('input-symptom-duration')?.value;
    if (duration) {
        symptoms.push('Duration: ' + duration);
    }
    const severity = document.querySelector('input[name="severity_level"]:checked')?.value;
    if (severity) {
        symptoms.push('Severity: ' + severity);
    }
    const customNotes = document.getElementById('input-custom-sym')?.value.trim();
    if (customNotes) {
        symptoms.push(customNotes);
    }
    // Collect family and lifestyle history
    const historyList = [];
    document.querySelectorAll('input[name="family_hist"]:checked').forEach(function (cb) {
        historyList.push(cb.value);
    });
    document.querySelectorAll('input[name="lifestyle_hist"]:checked').forEach(function (cb) {
        historyList.push(cb.value);
    });
    const priorMed = document.getElementById('input-history')?.value.trim();
    if (priorMed) {
        historyList.push(priorMed);
    }
    // Build assessment payload (without clinical vitals & measurements)
    const payload = {
        patient_id: user ? user.id : 2,
        patient_name: user ? user.full_name : 'Patient',
        age: user ? user.age : 45,
        gender: user ? user.gender : 'Not Specified',
        phone: user ? user.phone : '+91 98765 00000',
        village_area: user ? user.village_area : 'Community Area',
        bp_systolic: 0,
        bp_diastolic: 0,
        blood_sugar: 0,
        heart_rate: 0,
        oxygen_level: 0,
        temperature: 0,
        symptoms_text: symptoms.length > 0 ? symptoms.join(', ') : 'None',
        medical_history: historyList.length > 0 ? historyList.join(', ') : 'No prior medical history'
    };
    try {
        const response = await fetch('/api/assessments', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        const data = await response.json();
        if (!response.ok) throw new Error(data.error || 'AI assessment failed');

        if (window.AuthNotificationModule) {
            window.AuthNotificationModule.notify('AI Health Assessment complete!', 'success');
        }
        // Close modal and reset form
        document.getElementById('assess-modal').classList.remove('active');
        if (form) form.reset();
        // Render Report on the patient page
        renderReport(data.assessment, false);
        // Refresh past assessments history
        if (window.DbHistoryModule && user) {
            window.DbHistoryModule.loadHistory(user.id);
        }
    } catch (err) {
        if (window.AuthNotificationModule) {
            window.AuthNotificationModule.notify(err.message, 'error');
        } else {
            alert(err.message);
        }
    } finally {
        submitBtn.disabled = false;
        submitBtn.textContent = '⚡ Run AI Diagnostic Assessment';
    }
}
