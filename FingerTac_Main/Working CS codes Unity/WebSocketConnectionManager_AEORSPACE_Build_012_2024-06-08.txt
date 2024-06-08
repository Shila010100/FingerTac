using UnityEngine;
using WebSocketSharp;

public class WebSocketConnectionManager : MonoBehaviour
{
    private string serverAddress = "ws://192.168.4.1:80/"; // Adjusted for AP mode
    public WebSocket ws;

    void Start()
    {
        ConnectWebSocket();
    }

    void ConnectWebSocket()
    {
        ws = new WebSocket(serverAddress);
        ws.OnOpen += OnOpenHandler;
        ws.OnMessage += OnMessageHandler;
        ws.OnError += OnErrorHandler;
        ws.OnClose += OnCloseHandler;

        Debug.Log("Attempting to connect to WebSocket server at " + serverAddress);
        try
        {
            ws.Connect();
        }
        catch (System.Exception ex)
        {
            Debug.LogError("WebSocket connection failed: " + ex.Message);
        }
    }

    private void OnOpenHandler(object sender, System.EventArgs e)
    {
        Debug.Log("WebSocket connected");
    }

    private void OnMessageHandler(object sender, MessageEventArgs e)
    {
        Debug.Log("Received message from server: " + e.Data);
    }

    private void OnErrorHandler(object sender, ErrorEventArgs e)
    {
        Debug.LogError("WebSocket error: " + e.Message);
    }

    private void OnCloseHandler(object sender, CloseEventArgs e)
    {
        Debug.Log("WebSocket closed");
    }

    void OnDestroy()
    {
        if (ws != null)
        {
            ws.Close();
        }
    }
}
