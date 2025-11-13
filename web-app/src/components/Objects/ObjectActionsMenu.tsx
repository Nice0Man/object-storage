import React from "react";
import {
  IconButton,
  Menu,
  MenuItem,
  ListItemIcon,
  ListItemText,
  Divider,
} from "@mui/material";
import {
  MoreVert,
  Info,
  Download,
  FileCopy,
  Label,
  Link,
  Delete,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";

interface ObjectActionsMenuProps {
  onInfo: () => void;
  onDownload: () => void;
  onCopy: () => void;
  onTags: () => void;
  onPresignedUrl: () => void;
  onDelete: () => void;
}

const ObjectActionsMenu: React.FC<ObjectActionsMenuProps> = ({
  onInfo,
  onDownload,
  onCopy,
  onTags,
  onPresignedUrl,
  onDelete,
}) => {
  const { t } = useTranslation();
  const [anchorEl, setAnchorEl] = React.useState<null | HTMLElement>(null);
  const open = Boolean(anchorEl);

  const handleClick = (event: React.MouseEvent<HTMLElement>) => {
    event.stopPropagation();
    setAnchorEl(event.currentTarget);
  };

  const handleClose = () => {
    setAnchorEl(null);
  };

  const handleAction = (action: () => void) => {
    action();
    handleClose();
  };

  return (
    <>
      <IconButton size="small" onClick={handleClick} aria-label="more actions">
        <MoreVert />
      </IconButton>
      <Menu
        anchorEl={anchorEl}
        open={open}
        onClose={handleClose}
        onClick={(e) => e.stopPropagation()}
        transformOrigin={{ horizontal: "right", vertical: "top" }}
        anchorOrigin={{ horizontal: "right", vertical: "bottom" }}
      >
        <MenuItem onClick={() => handleAction(onInfo)}>
          <ListItemIcon>
            <Info fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.info")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onDownload)}>
          <ListItemIcon>
            <Download fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.download")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onCopy)}>
          <ListItemIcon>
            <FileCopy fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.copy")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onTags)}>
          <ListItemIcon>
            <Label fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.tags")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onPresignedUrl)}>
          <ListItemIcon>
            <Link fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.generateLink")}</ListItemText>
        </MenuItem>
        <Divider />
        <MenuItem
          onClick={() => handleAction(onDelete)}
          sx={{ color: "error.main" }}
        >
          <ListItemIcon>
            <Delete fontSize="small" color="error" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.delete")}</ListItemText>
        </MenuItem>
      </Menu>
    </>
  );
};

export default ObjectActionsMenu;
