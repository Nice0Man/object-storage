import React, { useState, useEffect, useCallback, useMemo } from "react";
import {
    Dialog,
    DialogContent,
    Box,
    Typography,
    IconButton,
    CircularProgress,
    Skeleton,
    Chip,
    Stack,
    Tooltip,
    Button,
    alpha,
    useTheme,
    Fade,
    Zoom,
} from "@mui/material";
import {
    Close,
    Download,
    ZoomIn,
    ZoomOut,
    RotateRight,
    Fullscreen,
    FullscreenExit,
    ContentCopy,
    Image as ImageIcon,
    VideoFile,
    AudioFile,
    PictureAsPdf,
    Description,
    Code,
    InsertDriveFile,
    SkipNext,
    SkipPrevious,
    Lock,
} from "@mui/icons-material";
import { PhotoProvider, PhotoView } from "react-photo-view";
import "react-photo-view/dist/react-photo-view.css";

// Supported file types
const IMAGE_TYPES = ["jpg", "jpeg", "png", "gif", "webp", "svg", "bmp", "ico"];
const VIDEO_TYPES = ["mp4", "webm", "ogg", "mov", "avi"];
const AUDIO_TYPES = ["mp3", "wav", "ogg", "aac", "flac", "m4a"];
const PDF_TYPES = ["pdf"];
const CODE_TYPES = ["js", "ts", "jsx", "tsx", "json", "xml", "html", "css", "scss", "less", "py", "java", "cpp", "c", "h", "hpp", "rs", "go", "rb", "php", "sh", "bash", "yaml", "yml", "toml", "ini", "conf", "md", "txt", "log", "sql", "graphql"];
const DOCUMENT_TYPES = ["doc", "docx", "xls", "xlsx", "ppt", "pptx", "odt", "ods", "odp"];

interface FilePreviewProps {
    open: boolean;
    onClose: () => void;
    fileUrl: string;
    fileName: string;
    fileSize?: number;
    isEncrypted?: boolean;
    encryptionType?: string;
    onDownload?: () => void;
    // For gallery mode
    files?: Array<{ url: string; name: string; size?: number }>;
    currentIndex?: number;
    onNavigate?: (index: number) => void;
}

const FilePreview: React.FC<FilePreviewProps> = ({
    open,
    onClose,
    fileUrl,
    fileName,
    fileSize,
    isEncrypted = false,
    encryptionType,
    onDownload,
    files,
    currentIndex = 0,
    onNavigate,
}) => {
    const theme = useTheme();
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);
    const [textContent, setTextContent] = useState<string>("");
    const [zoom, setZoom] = useState(1);
    const [rotation, setRotation] = useState(0);
    const [isFullscreen, setIsFullscreen] = useState(false);
    const [isPlaying, setIsPlaying] = useState(false);
    const [copied, setCopied] = useState(false);

    // Get file extension
    const fileExtension = useMemo(() => {
        const ext = fileName.split(".").pop()?.toLowerCase() || "";
        return ext;
    }, [fileName]);

    // Determine file type category
    const fileType = useMemo(() => {
        if (IMAGE_TYPES.includes(fileExtension)) return "image";
        if (VIDEO_TYPES.includes(fileExtension)) return "video";
        if (AUDIO_TYPES.includes(fileExtension)) return "audio";
        if (PDF_TYPES.includes(fileExtension)) return "pdf";
        if (CODE_TYPES.includes(fileExtension)) return "code";
        if (DOCUMENT_TYPES.includes(fileExtension)) return "document";
        return "unknown";
    }, [fileExtension]);

    // Get file icon based on type
    const FileIcon = useMemo(() => {
        switch (fileType) {
            case "image": return ImageIcon;
            case "video": return VideoFile;
            case "audio": return AudioFile;
            case "pdf": return PictureAsPdf;
            case "code": return Code;
            case "document": return Description;
            default: return InsertDriveFile;
        }
    }, [fileType]);

    // Format file size
    const formatSize = (bytes?: number) => {
        if (!bytes) return "";
        const k = 1024;
        const sizes = ["B", "KB", "MB", "GB"];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return `${(bytes / Math.pow(k, i)).toFixed(1)} ${sizes[i]}`;
    };

    // Load text content for code files
    useEffect(() => {
        if (open && fileType === "code" && fileUrl) {
            setLoading(true);
            setError(null);
            fetch(fileUrl)
                .then((res) => {
                    if (!res.ok) throw new Error("Failed to load file");
                    return res.text();
                })
                .then((text) => {
                    setTextContent(text);
                    setLoading(false);
                })
                .catch((err) => {
                    setError(err.message);
                    setLoading(false);
                });
        }
    }, [open, fileType, fileUrl]);

    // Reset state when closing
    useEffect(() => {
        if (!open) {
            setZoom(1);
            setRotation(0);
            setLoading(true);
            setError(null);
            setTextContent("");
        }
    }, [open]);

    // Handle keyboard navigation
    useEffect(() => {
        if (!open || !files || files.length <= 1) return;

        const handleKeyDown = (e: KeyboardEvent) => {
            if (e.key === "ArrowLeft" && currentIndex > 0) {
                onNavigate?.(currentIndex - 1);
            } else if (e.key === "ArrowRight" && currentIndex < files.length - 1) {
                onNavigate?.(currentIndex + 1);
            } else if (e.key === "Escape") {
                onClose();
            }
        };

        window.addEventListener("keydown", handleKeyDown);
        return () => window.removeEventListener("keydown", handleKeyDown);
    }, [open, files, currentIndex, onNavigate, onClose]);

    const handleZoomIn = () => setZoom((z) => Math.min(z + 0.25, 3));
    const handleZoomOut = () => setZoom((z) => Math.max(z - 0.25, 0.5));
    const handleRotate = () => setRotation((r) => (r + 90) % 360);
    const handleFullscreen = () => setIsFullscreen((f) => !f);

    const handleCopyContent = useCallback(() => {
        if (textContent) {
            navigator.clipboard.writeText(textContent);
            setCopied(true);
            setTimeout(() => setCopied(false), 2000);
        }
    }, [textContent]);

    // Render image preview
    const renderImage = () => (
        <Box
            sx={{
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                height: "100%",
                overflow: "hidden",
                bgcolor: alpha(theme.palette.common.black, 0.9),
            }}
        >
            {loading && (
                <Skeleton
                    variant="rectangular"
                    width="80%"
                    height="60%"
                    animation="wave"
                    sx={{ borderRadius: 2 }}
                />
            )}
            <PhotoProvider
                maskOpacity={0.95}
                bannerVisible={true}
                toolbarRender={({ rotate, onRotate, onScale, scale }) => (
                    <Stack direction="row" spacing={1}>
                        <IconButton size="small" onClick={() => onScale(scale + 0.5)} sx={{ color: "white" }}>
                            <ZoomIn />
                        </IconButton>
                        <IconButton size="small" onClick={() => onScale(scale - 0.5)} sx={{ color: "white" }}>
                            <ZoomOut />
                        </IconButton>
                        <IconButton size="small" onClick={() => onRotate(rotate + 90)} sx={{ color: "white" }}>
                            <RotateRight />
                        </IconButton>
                    </Stack>
                )}
            >
                <PhotoView src={fileUrl}>
                    <img
                        src={fileUrl}
                        alt={fileName}
                        style={{
                            maxWidth: "100%",
                            maxHeight: "100%",
                            objectFit: "contain",
                            transform: `scale(${zoom}) rotate(${rotation}deg)`,
                            transition: "transform 0.3s ease",
                            cursor: "zoom-in",
                            display: loading ? "none" : "block",
                        }}
                        onLoad={() => setLoading(false)}
                        onError={() => {
                            setError("Failed to load image");
                            setLoading(false);
                        }}
                    />
                </PhotoView>
            </PhotoProvider>
        </Box>
    );

    // Render video preview
    const renderVideo = () => (
        <Box
            sx={{
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                height: "100%",
                bgcolor: alpha(theme.palette.common.black, 0.95),
            }}
        >
            <video
                src={fileUrl}
                controls
                autoPlay={false}
                style={{
                    maxWidth: "100%",
                    maxHeight: "100%",
                    borderRadius: 8,
                }}
                onLoadedData={() => setLoading(false)}
                onError={() => {
                    setError("Failed to load video");
                    setLoading(false);
                }}
                onPlay={() => setIsPlaying(true)}
                onPause={() => setIsPlaying(false)}
            />
        </Box>
    );

    // Render audio preview
    const renderAudio = () => (
        <Box
            sx={{
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                justifyContent: "center",
                height: "100%",
                bgcolor: `linear-gradient(135deg, ${alpha(theme.palette.primary.dark, 0.3)} 0%, ${alpha(theme.palette.secondary.dark, 0.3)} 100%)`,
                background: theme.palette.mode === "dark"
                    ? "linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%)"
                    : "linear-gradient(135deg, #667eea 0%, #764ba2 100%)",
                p: 4,
            }}
        >
            <Fade in={!loading}>
                <Box sx={{ textAlign: "center" }}>
                    <Box
                        sx={{
                            width: 180,
                            height: 180,
                            borderRadius: "50%",
                            bgcolor: alpha(theme.palette.common.white, 0.1),
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                            mb: 3,
                            boxShadow: `0 0 60px ${alpha(theme.palette.primary.main, 0.3)}`,
                            animation: isPlaying ? "pulse 2s ease-in-out infinite" : "none",
                            "@keyframes pulse": {
                                "0%, 100%": { transform: "scale(1)" },
                                "50%": { transform: "scale(1.05)" },
                            },
                        }}
                    >
                        <AudioFile sx={{ fontSize: 80, color: "white" }} />
                    </Box>
                    <Typography variant="h6" sx={{ color: "white", mb: 1 }}>
                        {fileName}
                    </Typography>
                    {fileSize && (
                        <Typography variant="caption" sx={{ color: alpha(theme.palette.common.white, 0.7) }}>
                            {formatSize(fileSize)}
                        </Typography>
                    )}
                    <Box sx={{ mt: 3, width: "100%", maxWidth: 400 }}>
                        <audio
                            src={fileUrl}
                            controls
                            style={{ width: "100%" }}
                            onLoadedData={() => setLoading(false)}
                            onError={() => {
                                setError("Failed to load audio");
                                setLoading(false);
                            }}
                            onPlay={() => setIsPlaying(true)}
                            onPause={() => setIsPlaying(false)}
                        />
                    </Box>
                </Box>
            </Fade>
        </Box>
    );

    // Render PDF preview
    const renderPdf = () => (
        <Box sx={{ height: "100%", width: "100%" }}>
            <iframe
                src={`${fileUrl}#view=FitH`}
                title={fileName}
                style={{
                    width: "100%",
                    height: "100%",
                    border: "none",
                }}
                onLoad={() => setLoading(false)}
                onError={() => {
                    setError("Failed to load PDF");
                    setLoading(false);
                }}
            />
        </Box>
    );

    // Render code/text preview
    const renderCode = () => (
        <Box
            sx={{
                height: "100%",
                overflow: "auto",
                bgcolor: theme.palette.mode === "dark" ? "#1e1e1e" : "#f5f5f5",
            }}
        >
            {loading ? (
                <Box sx={{ p: 3 }}>
                    {[...Array(20)].map((_, i) => (
                        <Skeleton key={i} variant="text" width={`${Math.random() * 60 + 40}%`} sx={{ mb: 0.5 }} />
                    ))}
                </Box>
            ) : error ? (
                <Box sx={{ p: 3, textAlign: "center" }}>
                    <Typography color="error">{error}</Typography>
                </Box>
            ) : (
                <Box sx={{ position: "relative" }}>
                    <Tooltip title={copied ? "Copied!" : "Copy content"}>
                        <IconButton
                            onClick={handleCopyContent}
                            sx={{
                                position: "absolute",
                                top: 8,
                                right: 8,
                                bgcolor: alpha(theme.palette.background.paper, 0.9),
                                "&:hover": { bgcolor: theme.palette.background.paper },
                            }}
                            size="small"
                        >
                            <ContentCopy fontSize="small" />
                        </IconButton>
                    </Tooltip>
                    <pre
                        style={{
                            margin: 0,
                            padding: 16,
                            fontFamily: "'Fira Code', 'Monaco', 'Consolas', monospace",
                            fontSize: 13,
                            lineHeight: 1.6,
                            whiteSpace: "pre-wrap",
                            wordBreak: "break-word",
                            color: theme.palette.text.primary,
                        }}
                    >
                        <code>{textContent}</code>
                    </pre>
                </Box>
            )}
        </Box>
    );

    // Render unsupported file type
    const renderUnsupported = () => (
        <Box
            sx={{
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                justifyContent: "center",
                height: "100%",
                p: 4,
                textAlign: "center",
            }}
        >
            <Zoom in={true}>
                <Box
                    sx={{
                        width: 120,
                        height: 120,
                        borderRadius: 3,
                        bgcolor: alpha(theme.palette.primary.main, 0.1),
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                        mb: 3,
                    }}
                >
                    <FileIcon sx={{ fontSize: 60, color: theme.palette.primary.main }} />
                </Box>
            </Zoom>
            <Typography variant="h6" sx={{ mb: 1 }}>
                {fileName}
            </Typography>
            <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                Preview not available for this file type
            </Typography>
            {fileSize && (
                <Chip
                    label={formatSize(fileSize)}
                    size="small"
                    variant="outlined"
                    sx={{ mb: 2 }}
                />
            )}
            {isEncrypted && (
                <Chip
                    icon={<Lock fontSize="small" />}
                    label={encryptionType || "Encrypted"}
                    size="small"
                    color="warning"
                    sx={{ mb: 2 }}
                />
            )}
            {onDownload && (
                <Button
                    variant="contained"
                    startIcon={<Download />}
                    onClick={onDownload}
                    sx={{ mt: 2 }}
                >
                    Download File
                </Button>
            )}
        </Box>
    );

    // Render content based on file type
    const renderContent = () => {
        if (error && fileType !== "code") {
            return (
                <Box
                    sx={{
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        justifyContent: "center",
                        height: "100%",
                        p: 4,
                    }}
                >
                    <Typography color="error" variant="h6" sx={{ mb: 2 }}>
                        Failed to load preview
                    </Typography>
                    <Typography color="text.secondary" sx={{ mb: 3 }}>
                        {error}
                    </Typography>
                    {onDownload && (
                        <Button variant="outlined" startIcon={<Download />} onClick={onDownload}>
                            Download instead
                        </Button>
                    )}
                </Box>
            );
        }

        switch (fileType) {
            case "image": return renderImage();
            case "video": return renderVideo();
            case "audio": return renderAudio();
            case "pdf": return renderPdf();
            case "code": return renderCode();
            default: return renderUnsupported();
        }
    };

    return (
        <Dialog
            open={open}
            onClose={onClose}
            maxWidth={false}
            fullScreen={isFullscreen}
            PaperProps={{
                sx: {
                    width: isFullscreen ? "100%" : "90vw",
                    height: isFullscreen ? "100%" : "85vh",
                    maxWidth: isFullscreen ? "100%" : 1400,
                    bgcolor: theme.palette.mode === "dark" ? "#0a0a0a" : "#fafafa",
                    borderRadius: isFullscreen ? 0 : 2,
                    overflow: "hidden",
                },
            }}
        >
            {/* Header */}
            <Box
                sx={{
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "space-between",
                    px: 2,
                    py: 1.5,
                    borderBottom: `1px solid ${theme.palette.divider}`,
                    bgcolor: alpha(theme.palette.background.paper, 0.95),
                    backdropFilter: "blur(8px)",
                }}
            >
                <Stack direction="row" spacing={2} alignItems="center">
                    <Box
                        sx={{
                            width: 40,
                            height: 40,
                            borderRadius: 1.5,
                            bgcolor: alpha(theme.palette.primary.main, 0.1),
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                        }}
                    >
                        <FileIcon sx={{ color: theme.palette.primary.main }} />
                    </Box>
                    <Box>
                        <Typography variant="subtitle1" sx={{ fontWeight: 600, lineHeight: 1.2 }}>
                            {fileName}
                        </Typography>
                        <Stack direction="row" spacing={1} alignItems="center">
                            {fileSize && (
                                <Typography variant="caption" color="text.secondary">
                                    {formatSize(fileSize)}
                                </Typography>
                            )}
                            {isEncrypted && (
                                <Chip
                                    icon={<Lock sx={{ fontSize: 12 }} />}
                                    label={encryptionType || "Encrypted"}
                                    size="small"
                                    sx={{
                                        height: 20,
                                        fontSize: 10,
                                        bgcolor: alpha(theme.palette.warning.main, 0.1),
                                        color: theme.palette.warning.main,
                                    }}
                                />
                            )}
                        </Stack>
                    </Box>
                </Stack>

                {/* Toolbar */}
                <Stack direction="row" spacing={0.5} alignItems="center">
                    {/* Gallery navigation */}
                    {files && files.length > 1 && (
                        <>
                            <Tooltip title="Previous (←)">
                                <span>
                                    <IconButton
                                        size="small"
                                        onClick={() => onNavigate?.(currentIndex - 1)}
                                        disabled={currentIndex === 0}
                                    >
                                        <SkipPrevious />
                                    </IconButton>
                                </span>
                            </Tooltip>
                            <Chip
                                label={`${currentIndex + 1} / ${files.length}`}
                                size="small"
                                variant="outlined"
                                sx={{ mx: 1 }}
                            />
                            <Tooltip title="Next (→)">
                                <span>
                                    <IconButton
                                        size="small"
                                        onClick={() => onNavigate?.(currentIndex + 1)}
                                        disabled={currentIndex === files.length - 1}
                                    >
                                        <SkipNext />
                                    </IconButton>
                                </span>
                            </Tooltip>
                            <Box sx={{ width: 16 }} />
                        </>
                    )}

                    {/* Image controls */}
                    {fileType === "image" && !loading && (
                        <>
                            <Tooltip title="Zoom in">
                                <IconButton size="small" onClick={handleZoomIn}>
                                    <ZoomIn />
                                </IconButton>
                            </Tooltip>
                            <Tooltip title="Zoom out">
                                <IconButton size="small" onClick={handleZoomOut}>
                                    <ZoomOut />
                                </IconButton>
                            </Tooltip>
                            <Tooltip title="Rotate">
                                <IconButton size="small" onClick={handleRotate}>
                                    <RotateRight />
                                </IconButton>
                            </Tooltip>
                            <Box sx={{ width: 8 }} />
                        </>
                    )}

                    <Tooltip title={isFullscreen ? "Exit fullscreen" : "Fullscreen"}>
                        <IconButton size="small" onClick={handleFullscreen}>
                            {isFullscreen ? <FullscreenExit /> : <Fullscreen />}
                        </IconButton>
                    </Tooltip>

                    {onDownload && (
                        <Tooltip title="Download">
                            <IconButton size="small" onClick={onDownload}>
                                <Download />
                            </IconButton>
                        </Tooltip>
                    )}

                    <Tooltip title="Close (Esc)">
                        <IconButton size="small" onClick={onClose} sx={{ ml: 1 }}>
                            <Close />
                        </IconButton>
                    </Tooltip>
                </Stack>
            </Box>

            {/* Content */}
            <DialogContent
                sx={{
                    p: 0,
                    height: "calc(100% - 64px)",
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                    overflow: "hidden",
                    position: "relative",
                }}
            >
                {loading && fileType !== "code" && (
                    <Box
                        sx={{
                            position: "absolute",
                            top: "50%",
                            left: "50%",
                            transform: "translate(-50%, -50%)",
                            zIndex: 10,
                        }}
                    >
                        <CircularProgress />
                    </Box>
                )}
                {renderContent()}
            </DialogContent>
        </Dialog>
    );
};

export default FilePreview;
