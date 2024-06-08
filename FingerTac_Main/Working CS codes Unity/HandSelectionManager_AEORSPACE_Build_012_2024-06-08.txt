using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;

public class HandSelectionManager : MonoBehaviour
{
    private List<SendVibrationOnHover> vibrationScripts; // List to hold multiple instances

    // These are the buttons in your UI
    public Button leftHandButton;
    public Button rightHandButton;

    void Start()
    {
        vibrationScripts = new List<SendVibrationOnHover>(FindObjectsOfType<SendVibrationOnHover>()); // Find all instances and add them to the list

        if (vibrationScripts.Count == 0)
        {
            Debug.LogError("No SendVibrationOnHover scripts found in the scene.");
        }

        leftHandButton.onClick.AddListener(() => SetHandMode(false)); // Set mode to left hand
        rightHandButton.onClick.AddListener(() => SetHandMode(true)); // Set mode to right hand
    }

    public void SetHandMode(bool isRightHand)
    {
        foreach (var script in vibrationScripts)
        {
            if (script != null)
            {
                script.SetHandMode(isRightHand);
            }
        }
        Debug.Log("Hand mode set to: " + (isRightHand ? "Right" : "Left"));
    }
}
